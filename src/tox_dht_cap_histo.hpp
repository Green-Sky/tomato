#pragma once

#include <solanaceae/toxcore/tox_private_interface.hpp>

#include <vector>

class ToxDHTCapHisto {
	ToxPrivateI& _tpi;

	bool _enabled {true};
	bool _show_window {false};

	std::vector<float> _ratios;
	const float _value_add_interval {1.f}; // every second
	float _time_since_last_add {0.f};

	public:
		ToxDHTCapHisto(ToxPrivateI& tpi) : _tpi(tpi) {}

		void tick(float time_delta);
		void render(void);
};
