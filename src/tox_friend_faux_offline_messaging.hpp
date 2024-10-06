#pragma once

#include <solanaceae/toxcore/tox_event_interface.hpp>
#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

// fwd
struct ToxI;
namespace Contact::Components {
	struct ToxFriendEphemeral;
}

// resends unconfirmed messages.
// timers get reset on connection changes, and send order is preserved.
class ToxFriendFauxOfflineMessaging : public ToxEventI {
	Contact3Registry& _cr;
	RegistryMessageModelI& _rmm;
	ToxContactModel2& _tcm;
	ToxI& _t;
	ToxEventProviderI& _tep;

	float _interval_timer{0.f};

	// TODO: increase timer?
	const float _delay_after_cc {4.5f};
	const float _delay_inbetween {0.3f};
	const float _delay_retry {10.f}; // retry sending after 10s

	public:
		ToxFriendFauxOfflineMessaging(
			Contact3Registry& cr,
			RegistryMessageModelI& rmm,
			ToxContactModel2& tcm,
			ToxI& t,
			ToxEventProviderI& tep
		);

		float tick(float time_delta);

	private:
		enum class dfmc_Ret {
			TOO_SOON,
			SENT_THIS_TICK,
			NO_MSG,
		};
		// only called for online friends
		// returns true if a message was sent
		// dont call this too often
		dfmc_Ret doFriendMessageCheck(const Contact3 c, const Contact::Components::ToxFriendEphemeral& tfe);

	protected:
		bool onToxEvent(const Tox_Event_Friend_Connection_Status* e) override;
};

