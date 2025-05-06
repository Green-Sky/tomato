#pragma once

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/contact/fwd.hpp>
#include <solanaceae/toxcore/tox_interface.hpp>

#include "./backends/std_fs.hpp"

#include <string>
#include <vector>


// RIP fishy c=<

struct ConfigModelI;
struct ToxKey;

class ToxAvatarManager : public ObjectStoreEventI {
	ObjectStore2& _os;
	ObjectStore2::SubscriptionReference _os_sr;
	ContactStore4I& _cs;
	ConfigModelI& _conf;
	ToxI& _t;

	Backends::STDFS _sb_tcs;

	struct AcceptEntry {
		ObjectHandle m;
		std::string file_path;
	};
	std::vector<AcceptEntry> _accept_queue;

	public:
		ToxAvatarManager(
			ObjectStore2& os,
			ContactStore4I& cs,
			ConfigModelI& conf,
			ToxI& t
		);

		void iterate(void);

	protected:
		// TODO: become backend and work in objects instead
		std::string getAvatarPath(const ToxKey& key) const;
		std::string getAvatarFileName(const ToxKey& key) const;
		void addAvatarFileToContact(const Contact4 c, const ToxKey& key);
		void clearAvatarFromContact(const Contact4 c);
		void checkObj(ObjectHandle o);

	protected: // os
		// on new obj, check for ToxTransferFriend and emplace own contact tracker
		bool onEvent(const ObjectStore::Events::ObjectConstruct& e) override;
		bool onEvent(const ObjectStore::Events::ObjectUpdate& e) override;
		bool onEvent(const ObjectStore::Events::ObjectDestory& e) override;
};

