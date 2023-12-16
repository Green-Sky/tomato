#include "./tox_client.hpp"

// meh, change this
#include <exception>
#include <system_error>
#include <toxencryptsave/toxencryptsave.h>

#include <sodium.h>

#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>

static void eee(std::string& mod) {
	for (char& c : mod) {
		c ^= 0x59;
	}
}

static void tmp_tox_log_cb(Tox *tox, Tox_Log_Level level, const char *file, uint32_t line, const char *func, const char *message, void *user_data) {
	std::cerr << "l:" << level << " " << file << ":" << line << "@" << func << ": '" << message << "'\n";
}

ToxClient::ToxClient(std::string_view save_path, std::string_view save_password) :
	_tox_profile_path(save_path), _tox_profile_password(save_password)
{
	TOX_ERR_OPTIONS_NEW err_opt_new;
	Tox_Options* options = tox_options_new(&err_opt_new);
	assert(err_opt_new == TOX_ERR_OPTIONS_NEW::TOX_ERR_OPTIONS_NEW_OK);

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
						throw std::runtime_error("FAILED to decrypt save file!!!!");
					}
					eee(_tox_profile_password);
				}
				tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
				tox_options_set_savedata_data(options, profile_data.data(), profile_data.size());
			}

			ifile.close(); // do i need this?
		}
	}

	tox_options_set_log_callback(options, tmp_tox_log_cb);

	TOX_ERR_NEW err_new;
	_tox = tox_new(options, &err_new);
	tox_options_free(options);
	if (err_new != TOX_ERR_NEW_OK) {
		std::cerr << "tox_new failed with error code " << err_new << "\n";
		throw std::runtime_error{"tox failed"};
	}

	// no callbacks, use events
	tox_events_init(_tox);

	// dht bootstrap
	{
		struct DHT_node {
			const char *ip;
			uint16_t port;
			const char key_hex[TOX_PUBLIC_KEY_SIZE*2 + 1]; // 1 for null terminator
			unsigned char key_bin[TOX_PUBLIC_KEY_SIZE];
		};

		DHT_node nodes[] =
		{
			// TODO: more/diff nodes
			// you can change or add your own bs and tcprelays here, ideally closer to you

			//{"tox.plastiras.org",	33445,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14
			{"104.244.74.69",		443,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14
			{"104.244.74.69",		33445,	"8E8B63299B3D520FB377FE5100E65E3322F7AE5B20A0ACED2981769FC5B43725", {}}, // LU tha14

			//{"tox2.plastiras.org",	33445,	"B6626D386BE7E3ACA107B46F48A5C4D522D29281750D44A0CBA6A2721E79C951", {}}, // DE tha14

			//{"tox4.plastiras.org",	33445,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14
			{"37.221.66.161",	443,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14
			{"37.221.66.161",	33445,	"836D1DA2BE12FE0E669334E437BE3FB02806F1528C2B2782113E0910C7711409", {}}, // MD tha14
		};

		for (size_t i = 0; i < sizeof(nodes)/sizeof(DHT_node); i ++) {
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
}

ToxClient::~ToxClient(void) {
	tox_kill(_tox);
}

bool ToxClient::iterate(void) {
	Tox_Err_Events_Iterate err_e_it = TOX_ERR_EVENTS_ITERATE_OK;
	auto* events = tox_events_iterate(_tox, false, &err_e_it);
	if (err_e_it == TOX_ERR_EVENTS_ITERATE_OK && events != nullptr) {
		_subscriber_raw(events);

		// forward events to event handlers
		dispatchEvents(events);
	}

	tox_events_free(events);

	if (_tox_profile_dirty) {
		saveToxProfile();
	}

	return true;
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
	std::ofstream ofile{_tox_profile_path, std::ios::binary};
	// TODO: improve
	for (const auto& ch : data) {
		ofile.put(ch);
	}

	_tox_profile_dirty = false;
}

