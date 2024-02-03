#include "./tox_friend_faux_offline_messaging.hpp"

#include <solanaceae/toxcore/tox_interface.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/tox_contacts/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/components.hpp>

#include <limits>
#include <cstdint>

//#include <iostream>

namespace Message::Components {
	struct LastSendAttempt {
		uint64_t ts {0};
	};
} // Message::Components

namespace Contact::Components {
	struct NextSendAttempt {
		uint64_t ts {0};
	};
} // Contact::Components

ToxFriendFauxOfflineMessaging::ToxFriendFauxOfflineMessaging(
	Contact3Registry& cr,
	RegistryMessageModel& rmm,
	ToxContactModel2& tcm,
	ToxI& t,
	ToxEventProviderI& tep
) : _cr(cr), _rmm(rmm), _tcm(tcm), _t(t), _tep(tep) {
	_tep.subscribe(this, Tox_Event_Type::TOX_EVENT_FRIEND_CONNECTION_STATUS);
}

float ToxFriendFauxOfflineMessaging::tick(float time_delta) {
	_interval_timer -= time_delta;
	if (_interval_timer > 0.f) {
		return std::max(_interval_timer, 0.001f); // TODO: min next timer
	}
	// interval ~ once per minute
	_interval_timer = 60.f;


	const uint64_t ts_now = Message::getTimeMS();

	// check ALL
	// for each online tox friend
	uint64_t min_next_attempt_ts {std::numeric_limits<uint64_t>::max()};
	_cr.view<Contact::Components::ToxFriendEphemeral, Contact::Components::ConnectionState>()
	.each([this, &min_next_attempt_ts, ts_now](const Contact3 c, const auto& tfe, const auto& cs) {
		if (cs.state == Contact::Components::ConnectionState::disconnected) {
			// cleanup
			if (_cr.all_of<Contact::Components::NextSendAttempt>(c)) {
				_cr.remove<Contact::Components::NextSendAttempt>(c);
				auto* mr = static_cast<const RegistryMessageModel&>(_rmm).get(c);
				if (mr != nullptr) {
					mr->storage<Message::Components::LastSendAttempt>().clear();
				}
			}
		} else {
			if (!_cr.all_of<Contact::Components::NextSendAttempt>(c)) {
				if (false) { // has unsent messages
					const auto& nsa = _cr.emplace<Contact::Components::NextSendAttempt>(c, ts_now + uint64_t(_delay_after_cc*1000)); // wait before first message is sent
					min_next_attempt_ts = std::min(min_next_attempt_ts, nsa.ts);
				}
			} else {
				auto ret = doFriendMessageCheck(c, tfe);
				if (ret == dfmc_Ret::SENT_THIS_TICK) {
					const auto ts = _cr.get<Contact::Components::NextSendAttempt>(c).ts = ts_now + uint64_t(_delay_inbetween*1000);
					min_next_attempt_ts = std::min(min_next_attempt_ts, ts);
				} else if (ret == dfmc_Ret::TOO_SOON) {
					// TODO: set to _delay_inbetween? prob expensive for no good reason
					min_next_attempt_ts = std::min(min_next_attempt_ts, _cr.get<Contact::Components::NextSendAttempt>(c).ts);
				} else {
					_cr.remove<Contact::Components::NextSendAttempt>(c);
				}
			}
		}
	});

	if (min_next_attempt_ts <= ts_now) {
		// we (probably) sent this iterate
		_interval_timer = 0.1f; // TODO: ugly magic
	} else if (min_next_attempt_ts == std::numeric_limits<uint64_t>::max()) {
		// nothing to sync or all offline that need syncing
	} else {
		_interval_timer = std::min(_interval_timer, (min_next_attempt_ts - ts_now) / 1000.f);
	}

	//std::cout << "TFFOM: iterate (i:" << _interval_timer << ")\n";

	return _interval_timer;
}

ToxFriendFauxOfflineMessaging::dfmc_Ret ToxFriendFauxOfflineMessaging::doFriendMessageCheck(const Contact3 c, const Contact::Components::ToxFriendEphemeral& tfe) {
	// walk all messages and check if
	// unacked message
	// timeouts for exising unacked messages expired (send)

	auto* mr = static_cast<const RegistryMessageModel&>(_rmm).get(c);
	if (mr == nullptr) {
		// no messages
		return dfmc_Ret::NO_MSG;
	}

	const uint64_t ts_now = Message::getTimeMS();

	// filter for unconfirmed messages

	// we assume sorted
	// ("reverse" iteration <.<)
	auto msg_view = mr->view<Message::Components::Timestamp>();
	bool valid_unsent {false};
	// we search for the oldest, not too recently sent, unconfirmed message
	for (auto it = msg_view.rbegin(), view_end = msg_view.rend(); it != view_end; it++) {
		const Message3 msg = *it;

		// require
		if (!mr->all_of<
				Message::Components::MessageText, // text only for now
				Message::Components::ContactTo
			>(msg)
		) {
			continue; // skip
		}

		// exclude
		if (mr->any_of<
				Message::Components::Remote::TimestampReceived // this acts like a tag, which is wrong in groups
			>(msg)
		) {
			continue; // skip
		}

		if (mr->get<Message::Components::ContactTo>(msg).c != c) {
			continue; // not outbound (in private)
		}

		valid_unsent = true;

		uint64_t msg_ts = msg_view.get<Message::Components::Timestamp>(msg).ts;
		if (mr->all_of<Message::Components::TimestampWritten>(msg)) {
			msg_ts = mr->get<Message::Components::TimestampWritten>(msg).ts;
		}
		if (mr->all_of<Message::Components::LastSendAttempt>(msg)) {
			const auto lsa = mr->get<Message::Components::LastSendAttempt>(msg).ts;
			if (lsa > msg_ts) {
				msg_ts = lsa;
			}
		}

		if (ts_now < (msg_ts + uint64_t(_delay_retry * 1000))) {
			// not time yet
			continue;
		}

		// it is time
		const auto [msg_id, _] = _t.toxFriendSendMessage(
			tfe.friend_number,
			(
				mr->all_of<Message::Components::TagMessageIsAction>(msg)
					? Tox_Message_Type::TOX_MESSAGE_TYPE_ACTION
					: Tox_Message_Type::TOX_MESSAGE_TYPE_NORMAL
			),
			mr->get<Message::Components::MessageText>(msg).text
		);

		// TODO: this is ugly
		mr->emplace_or_replace<Message::Components::LastSendAttempt>(msg, ts_now);

		if (msg_id.has_value()) {
			// tmm will pick this up for us
			mr->emplace_or_replace<Message::Components::ToxFriendMessageID>(msg, msg_id.value());
		} // else error

		// we sent our message, no point further iterating
		return dfmc_Ret::SENT_THIS_TICK;
	}

	if (!valid_unsent) {
		// somehow cleanup lsa
		mr->storage<Message::Components::LastSendAttempt>().clear();
		//std::cout << "TFFOM: all sent, deleting lsa\n";
		return dfmc_Ret::NO_MSG;
	}

	return dfmc_Ret::TOO_SOON;
}

bool ToxFriendFauxOfflineMessaging::onToxEvent(const Tox_Event_Friend_Connection_Status* e) {
	const auto friend_number = tox_event_friend_connection_status_get_friend_number(e);
	const auto friend_status = tox_event_friend_connection_status_get_connection_status(e);

	if (friend_status == Tox_Connection::TOX_CONNECTION_NONE) {
		return false; // skip
		// maybe cleanup?
	}

	auto c = _tcm.getContactFriend(friend_number);
	if (!static_cast<bool>(c) || !c.all_of<Contact::Components::ToxFriendEphemeral, Contact::Components::ConnectionState>()) {
		// UH error??
		return false;
	}

	_cr.emplace_or_replace<Contact::Components::NextSendAttempt>(c, Message::getTimeMS() + uint64_t(_delay_after_cc*1000)); // wait before first message is sent

	_interval_timer = 0.f;

	return false;
}

