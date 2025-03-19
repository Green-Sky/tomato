#pragma once

#include "./tox_private_impl.hpp"

#include <cstdint>
#include <vector>
#include <map>

class ToxNetprofUI {
	ToxPrivateImpl& _tpi;

	bool _enabled {true};
	bool _show_window_table {false};
	bool _show_window_graph {false};

	// table delta
	std::map<uint8_t, uint64_t> _udp_ctx_prev;
	std::map<uint8_t, uint64_t> _udp_crx_prev;
	std::map<uint8_t, uint64_t> _udp_btx_prev;
	std::map<uint8_t, uint64_t> _udp_brx_prev;
	std::map<uint8_t, uint64_t> _tcp_ctx_prev;
	std::map<uint8_t, uint64_t> _tcp_crx_prev;
	std::map<uint8_t, uint64_t> _tcp_btx_prev;
	std::map<uint8_t, uint64_t> _tcp_brx_prev;

	// table heat
	std::map<uint8_t, float> _udp_ctx_heat;
	std::map<uint8_t, float> _udp_crx_heat;
	std::map<uint8_t, float> _udp_btx_heat;
	std::map<uint8_t, float> _udp_brx_heat;
	std::map<uint8_t, float> _tcp_ctx_heat;
	std::map<uint8_t, float> _tcp_crx_heat;
	std::map<uint8_t, float> _tcp_btx_heat;
	std::map<uint8_t, float> _tcp_brx_heat;

	// graph totals
	uint64_t _udp_tbtx_prev;
	uint64_t _udp_tbrx_prev;
	std::vector<float> _udp_tbtx;
	std::vector<float> _udp_tbrx;

	uint64_t _tcp_tbtx_prev;
	uint64_t _tcp_tbrx_prev;
	std::vector<float> _tcp_tbtx;
	std::vector<float> _tcp_tbrx;

	float _udp_tbtx_avg {0.f};
	float _udp_tbrx_avg {0.f};

	float _tcp_tbtx_avg {0.f};
	float _tcp_tbrx_avg {0.f};

	const float _value_add_interval {1.f}; // every second
	float _time_since_last_add {0.f};

	public:
		ToxNetprofUI(ToxPrivateImpl& tpi) : _tpi(tpi) {}

		void tick(float time_delta);
		float render(float time_delta);
};
