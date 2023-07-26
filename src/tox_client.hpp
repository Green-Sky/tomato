#pragma once

#include <solanaceae/toxcore/tox_default_impl.hpp>
#include <solanaceae/toxcore/tox_event_interface.hpp>
#include <solanaceae/toxcore/tox_event_provider_base.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <functional>

struct ToxEventI;

class ToxClient : public ToxDefaultImpl, public ToxEventProviderBase {
	private:
		bool _should_stop {false};

		std::function<void(const Tox_Events*)> _subscriber_raw {[](const auto*) {}}; // only 1?

		std::string _self_name;

		std::string _tox_profile_path;
		bool _tox_profile_dirty {true}; // set in callbacks

	public:
		//ToxClient(/*const CommandLine& cl*/);
		ToxClient(std::string_view save_path);
		~ToxClient(void);

	public: // tox stuff
		Tox* getTox(void) { return _tox; }

		void setDirty(void) { _tox_profile_dirty = true; }

		// returns false when we shoul stop the program
		bool iterate(void);
		void stop(void); // let it know it should exit

		void setToxProfilePath(const std::string& new_path) { _tox_profile_path = new_path; }
		void setSelfName(std::string_view new_name) { _self_name = new_name; toxSelfSetName(new_name); }

	public: // raw events
		void subscribeRaw(std::function<void(const Tox_Events*)> fn);

	private:
		void saveToxProfile(void);
};


