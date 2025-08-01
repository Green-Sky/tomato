#include "./tox_client.hpp"

// meh, change this
#include <exception>
#include <memory>
#include <system_error>
#include <tox/toxencryptsave.h>

#include <sodium.h>

#include <vector>
#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <cassert>

static void eee(std::string& mod) {
	for (char& c : mod) {
		c ^= 0x37;
	}
}

ToxClient::ToxClient(ConfigModelI& conf, std::string_view save_path, std::string_view save_password) :
	_tox_profile_path(save_path), _tox_profile_password(save_password)
{
	TOX_ERR_OPTIONS_NEW err_opt_new;
	std::unique_ptr<Tox_Options, decltype(&tox_options_free)> options {tox_options_new(&err_opt_new), &tox_options_free};
	assert(err_opt_new == TOX_ERR_OPTIONS_NEW::TOX_ERR_OPTIONS_NEW_OK);
	std::string tmp_proxy_host; // the string needs to survive until options is freed

	std::vector<uint8_t> profile_data{};
	if (!_tox_profile_path.empty()) {
		std::ifstream ifile{_tox_profile_path, std::ios::binary};

		if (ifile.is_open()) {
			std::cout << "TOX loading save " << _tox_profile_path << "\n";
			// fill savedata
			while (ifile.good()) {
				auto ch = ifile.get();
				if (ch == EOF) {
					break;
				} else {
					profile_data.push_back(ch);
				}
			}

			if (profile_data.empty()) {
				std::cerr << "empty tox save\n";
			} else {
				// set options
				if (!save_password.empty()) {
					std::vector<uint8_t> encrypted_copy(profile_data.begin(), profile_data.end());
					//profile_data.clear();
					profile_data.resize(encrypted_copy.size() - TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
					if (!tox_pass_decrypt(
						encrypted_copy.data(), encrypted_copy.size(),
						reinterpret_cast<const uint8_t*>(save_password.data()), save_password.size(),
						profile_data.data(),
						nullptr // TODO: error checking
					)) {
						throw std::runtime_error("failed to decrypt save file!");
					}
					eee(_tox_profile_password);
				}
				tox_options_set_savedata_type(options.get(), TOX_SAVEDATA_TYPE_TOX_SAVE);
				tox_options_set_savedata_data(options.get(), profile_data.data(), profile_data.size());
			}

			ifile.close(); // do i need this?
		}
	}

	tox_options_set_ipv6_enabled(options.get(), conf.get_bool("tox", "ipv6_enabled").value_or(true));
	tox_options_set_udp_enabled(options.get(), conf.get_bool("tox", "udp_enabled").value_or(true));
	tox_options_set_local_discovery_enabled(options.get(), conf.get_bool("tox", "local_discovery_enabled").value_or(true));
	//tox_options_set_dht_announcements_enabled(options.get(), conf.get_bool("tox", "dht_announcements_enabled").value_or(true));
	tox_options_set_dht_announcements_enabled(options.get(), true); // dont harm the network

	const size_t proxy_conf_sum = conf.has_string("tox", "proxy_type")
		+ conf.has_string("tox", "proxy_host")
		+ conf.has_int("tox", "proxy_port")
	;
	// if all proxy parts defined
	if (proxy_conf_sum == 3) {
		const std::string_view proxy_type_str = conf.get_string("tox", "proxy_type").value();
		if (proxy_type_str == "HTTP") {
			tox_options_set_proxy_type(options.get(), Tox_Proxy_Type::TOX_PROXY_TYPE_HTTP);
		} else if (proxy_type_str == "SOCKS5") {
			tox_options_set_proxy_type(options.get(), Tox_Proxy_Type::TOX_PROXY_TYPE_SOCKS5);
		} else {
			throw std::runtime_error("invalid proxy type in config");
		}

		tmp_proxy_host = conf.get_string("tox", "proxy_host").value();
		tox_options_set_proxy_host(options.get(), tmp_proxy_host.c_str());

		tox_options_set_proxy_port(options.get(), conf.get_int("tox", "proxy_port").value());
	} else if (proxy_conf_sum > 0) {
		throw std::runtime_error("config only partly specified proxy");
	}

	tox_options_set_start_port(options.get(), conf.get_int("tox", "start_port").value_or(0));
	tox_options_set_end_port(options.get(), conf.get_int("tox", "end_port").value_or(0));
	tox_options_set_tcp_port(options.get(), conf.get_int("tox", "tcp_port").value_or(0));
	tox_options_set_hole_punching_enabled(options.get(), conf.get_bool("tox", "hole_punching_enabled").value_or(true));

	tox_options_set_experimental_groups_persistence(options.get(), true);

	// annoyingly the inverse
	tox_options_set_experimental_disable_dns(options.get(), !conf.get_bool("tox", "dns").value_or(false));

	Tox_Err_New err_new;
	_tox = tox_new(options.get(), &err_new);
	if (err_new != TOX_ERR_NEW_OK) {
		std::cerr << "tox_new failed with error code " << err_new << "\n";
		throw std::runtime_error{std::string{"toxcore creation failed with '"} + tox_err_new_to_string(err_new) + "'"};
	}

	// no callbacks, use events
	tox_events_init(_tox);

	runBootstrap();
}

ToxClient::~ToxClient(void) {
	if (_tox_profile_dirty) {
		saveToxProfile();
	}
	tox_kill(_tox);
}

bool ToxClient::iterate(float time_delta) {
	Tox_Err_Events_Iterate err_e_it = TOX_ERR_EVENTS_ITERATE_OK;
	auto* events = tox_events_iterate(_tox, false, &err_e_it);
	if (err_e_it == TOX_ERR_EVENTS_ITERATE_OK && events != nullptr) {
		_subscriber_raw(events);

		// forward events to event handlers
		dispatchEvents(events);
	}

	tox_events_free(events);

	_save_heat -= time_delta;
	if (_tox_profile_dirty && _save_heat <= 0.f) {
		saveToxProfile();
	}

	return true;
}

void ToxClient::runBootstrap(void) {
	// TODO: extend and read from json?
	// TODO: seperate out relays
	struct DHT_node {
		const char *ip;
		uint16_t port;
		const char key_hex[TOX_PUBLIC_KEY_SIZE*2 + 1]; // 1 for null terminator
		unsigned char key_bin[TOX_PUBLIC_KEY_SIZE];
	};

	DHT_node nodes[] {
		// TODO: more/diff nodes
		// you can change or add your own bs and tcprelays here, ideally closer to you

		//{"tox.plastiras.org",	33445,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14
		{"104.244.74.69",		443,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14
		{"104.244.74.69",		33445,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14

		//{"tox2.plastiras.org",	33445,	"B6626D386BE7E3ACA107B46F48A5C4D522D29281750D44A0CBA6A2721E79C951", {}}, // DE tha14

		//{"tox4.plastiras.org",	33445,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14
		{"217.156.65.161",	443,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14
		{"217.156.65.161",	33445,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14

		{"141.11.229.155",	33445,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // US lzk
	};

	for (size_t i = 0; i < sizeof(nodes)/sizeof(DHT_node); i++) {
		sodium_hex2bin(
			nodes[i].key_bin, sizeof(nodes[i].key_bin),
			nodes[i].key_hex, sizeof(nodes[i].key_hex)-1,
			NULL, NULL, NULL
		);
		tox_bootstrap(_tox, nodes[i].ip, nodes[i].port, nodes[i].key_bin, NULL);
		// TODO: use extra tcp option to avoid error msgs
		// ... this is hardcore
		tox_add_tcp_relay(_tox, nodes[i].ip, nodes[i].port, nodes[i].key_bin, NULL);
	}
}

void ToxClient::subscribeRaw(std::function<void(const Tox_Events*)> fn) {
	_subscriber_raw = fn;
}

void ToxClient::saveToxProfile(void) {
	if (_tox_profile_path.empty()) {
		return;
	}
	std::cout << "TOX saving\n";

	std::vector<uint8_t> data{};
	data.resize(tox_get_savedata_size(_tox));
	tox_get_savedata(_tox, data.data());

	if (!_tox_profile_password.empty()) {
		std::vector<uint8_t> unencrypted_copy(data.begin(), data.end());
		//profile_data.clear();
		data.resize(unencrypted_copy.size() + TOX_PASS_ENCRYPTION_EXTRA_LENGTH);
		eee(_tox_profile_password);
		if (!tox_pass_encrypt(
			unencrypted_copy.data(), unencrypted_copy.size(),
			reinterpret_cast<const uint8_t*>(_tox_profile_password.data()), _tox_profile_password.size(),
			data.data(),
			nullptr // TODO: error checking
		)) {
			eee(_tox_profile_password);
			throw std::runtime_error("FAILED to encrypt save file!!!!");
		}
		eee(_tox_profile_password);
	}

	std::filesystem::path tmp_path = _tox_profile_path + ".tmp";
	tmp_path.replace_filename("." + tmp_path.filename().generic_u8string());

	try {
		std::ofstream ofile{tmp_path, std::ios::binary};
		ofile.write(reinterpret_cast<const char*>(data.data()), data.size());
		if (!ofile.good()) {
			// TODO: maybe enable fstream exceptions instead?
			throw std::runtime_error("write error");
		}
	} catch (...) {
		std::filesystem::remove(tmp_path);
		std::cerr << "TOX saving failed!\n";
		_save_heat = 10.f;
		return;
	}

	std::filesystem::rename(
		tmp_path,
		_tox_profile_path
	);

	_tox_profile_dirty = false;
	_save_heat = 10.f;
}

