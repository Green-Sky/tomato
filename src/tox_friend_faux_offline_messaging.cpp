#include "./tox_friend_faux_offline_messaging.hpp"

#include <solanaceae/toxcore/tox_interface.hpp>

#include <solanaceae/contact/components.hpp>
#include <solanaceae/tox_contacts/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/tox_messages/components.hpp>

#include <limits>
#include <cstdint>

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
}

float ToxFriendFauxOfflineMessaging::tick(float time_delta) {
	// hard limit interval to once per minute
	_interval_timer += time_delta;
	if (_interval_timer < 1.f * 60.f) {
		return std::max(60.f - _interval_timer, 0.001f); // TODO: min next timer
	}
	_interval_timer = 0.f;

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
			}
		} else {
			if (!_cr.all_of<Contact::Components::NextSendAttempt>(c)) {
				const auto& nsa = _cr.emplace<Contact::Components::NextSendAttempt>(c, ts_now + uint64_t(_delay_after_cc*1000)); // wait before first message is sent
				min_next_attempt_ts = std::min(min_next_attempt_ts, nsa.ts);
			} else {
				auto& next_attempt = _cr.get<Contact::Components::NextSendAttempt>(c).ts;

				if (doFriendMessageCheck(c, tfe)) {
					next_attempt = ts_now + uint64_t(_delay_inbetween*1000);
				}

				min_next_attempt_ts = std::min(min_next_attempt_ts, next_attempt);
			}
		}
	});

	if (min_next_attempt_ts <= ts_now) {
		// we (probably) sent this iterate
		_interval_timer = 60.f - 0.1f; // TODO: ugly magic
		return 0.1f;
	} else if (min_next_attempt_ts == std::numeric_limits<uint64_t>::max()) {
		// nothing to sync or all offline that need syncing
		return 60.f; // TODO: ugly magic
	} else {
		// TODO: ugly magic
		return _interval_timer = 60.f - std::min(60.f, (min_next_attempt_ts - ts_now) / 1000.f);
	}
}

bool ToxFriendFauxOfflineMessaging::doFriendMessageCheck(const Contact3 c, const Contact::Components::ToxFriendEphemeral& tfe) {
	// walk all messages and check if
	// unacked message
	// timeouts for exising unacked messages expired (send)

	auto* mr = static_cast<const RegistryMessageModel&>(_rmm).get(c);
	if (mr == nullptr) {
		// no messages
		return false;
	}

	const uint64_t ts_now = Message::getTimeMS();

	// filter for unconfirmed messages

	// we assume sorted
	// ("reverse" iteration <.<)
	auto msg_view = mr->view<Message::Components::Timestamp>();
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
		return true;
	}

	// TODO: somehow cleanup lsa

	return false;
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

	// TODO: ugly magic
	_interval_timer = 60.f - 0.1f;

	return false;
}

