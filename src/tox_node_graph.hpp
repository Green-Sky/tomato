#pragma once

#include <solanaceae/toxcore/tox_event_interface.hpp>

#include <solanaceae/toxcore/tox_private_interface.hpp>

#include <solanaceae/toxcore/tox_key.hpp>

#include <map>
#include <set>
#include <random>

class ToxNodeGraph final : public ToxEventI {
	ToxPrivateI* _tpi {nullptr};
	ToxEventProviderI::SubscriptionReference _tep_sr;

	bool _enabled {true};
	bool _show_window_graph {false};

	struct Node {
		std::set<std::string> ips;
		// TODO: ports?

		std::set<ToxKey> announced;

		float x {};
		float y {};

		// hidden dims
		float h0 {};
	};
	std::map<ToxKey, Node> _nodes;

	float _graph_size_x {1.f};
	float _graph_size_y {1.f};
	std::minstd_rand _rng {1337};

	uint64_t _sim_steps {4}; // per frame
	float _squeeze_hidden {0.15f};

	public:
		ToxNodeGraph(ToxEventProviderI& tep, ToxPrivateI* tpi = nullptr);
		~ToxNodeGraph(void) override;

		//void tick(float time_delta);
		float render(float time_delta);

	protected: // tox event
		bool onToxEvent(const Tox_Event_Dht_Nodes_Response* e) override;
};
