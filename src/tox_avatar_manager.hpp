#pragma once

#include "solanaceae/contact/contact_model3.hpp"
#include <solanaceae/message3/registry_message_model.hpp>


// RIP fishy c=<

struct ConfigModelI;
struct ToxKey;

class ToxAvatarManager : public RegistryMessageModelEventI {
	RegistryMessageModel& _rmm;
	Contact3Registry& _cr;
	ConfigModelI& _conf;

	struct AcceptEntry {
		Message3Handle m;
		std::string file_path;
	};
	std::vector<AcceptEntry> _accept_queue;

	public:
		ToxAvatarManager(
			RegistryMessageModel& rmm,
			Contact3Registry& cr,
			ConfigModelI& conf
		);

		void iterate(void);

	protected:
		std::string getAvatarPath(const ToxKey& key) const;
		void addAvatarFileToContact(const Contact3 c, const ToxKey& key);
		void clearAvatarFromContact(const Contact3 c);
		void checkMsg(Message3Handle h);

	protected: // mm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;
};

