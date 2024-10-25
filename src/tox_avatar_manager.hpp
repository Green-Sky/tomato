#pragma once

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/contact/contact_model3.hpp>

#include <string>
#include <vector>


// RIP fishy c=<

struct ConfigModelI;
struct ToxKey;

class ToxAvatarManager : public ObjectStoreEventI {
	ObjectStore2& _os;
	ObjectStore2::SubscriptionReference _os_sr;
	Contact3Registry& _cr;
	ConfigModelI& _conf;

	struct AcceptEntry {
		ObjectHandle m;
		std::string file_path;
	};
	std::vector<AcceptEntry> _accept_queue;

	public:
		ToxAvatarManager(
			ObjectStore2& os,
			Contact3Registry& cr,
			ConfigModelI& conf
		);

		void iterate(void);

	protected:
		// TODO: become backend and work in objects instead
		std::string getAvatarPath(const ToxKey& key) const;
		void addAvatarFileToContact(const Contact3 c, const ToxKey& key);
		void clearAvatarFromContact(const Contact3 c);
		void checkObj(ObjectHandle o);

	protected: // os
		bool onEvent(const ObjectStore::Events::ObjectConstruct& e) override;
		bool onEvent(const ObjectStore::Events::ObjectUpdate& e) override;
};

