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
};
