#pragma once

#include <tox/tox_private.h>
#include <solanaceae/toxcore/tox_private_interface.hpp>

struct ToxPrivateImpl : public ToxPrivateI {
	Tox* _tox = nullptr;

	ToxPrivateImpl(Tox* tox) : _tox(tox) {}
	virtual ~ToxPrivateImpl(void) {}

	uint16_t toxDHTGetNumCloselist(void) override {
		return tox_dht_get_num_closelist(_tox);
	}

	uint16_t toxDHTGetNumCloselistAnnounceCapable(void) override {
		return tox_dht_get_num_closelist_announce_capable(_tox);
	}

	std::tuple<std::optional<std::string>, Tox_Err_Group_Peer_Query> toxGroupPeerGetIPAddress(uint32_t group_number, uint32_t peer_id) override {
		Tox_Err_Group_Peer_Query err = TOX_ERR_GROUP_PEER_QUERY_OK;
		size_t str_size = tox_group_peer_get_ip_address_size(_tox, group_number, peer_id, &err);
		if (err != TOX_ERR_GROUP_PEER_QUERY_OK) {
			return {std::nullopt, err};
		}
		std::string ip_str(str_size, '\0');

		tox_group_peer_get_ip_address(_tox, group_number, peer_id, reinterpret_cast<uint8_t*>(ip_str.data()), &err);
		if (err == TOX_ERR_GROUP_PEER_QUERY_OK) {
			return {ip_str, err};
		} else {
			return {std::nullopt, err};
		}
	}
};
