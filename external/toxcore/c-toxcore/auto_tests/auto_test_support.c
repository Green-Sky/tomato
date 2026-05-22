#include <assert.h>  // assert
#include <stdlib.h>  // calloc, free

#include "check_compat.h"
#include "../testing/misc_tools.h"
#include "../toxcore/Messenger.h"
#include "../toxcore/mono_time.h"
#include "../toxcore/tox_dispatch.h"
#include "../toxcore/tox_events.h"
#include "../toxcore/tox_struct.h"
#include "../toxcore/net_crypto.h"
#include "../toxcore/DHT.h"

#include "auto_test_support.h"

#ifndef ABORT_ON_LOG_ERROR
#define ABORT_ON_LOG_ERROR true
#endif

static const uint8_t *auto_test_nc_dht_get_shared_key_sent_wrapper(void *dht, const uint8_t *public_key)
{
    return dht_get_shared_key_sent((DHT *)dht, public_key);
}

static const uint8_t *auto_test_nc_dht_get_self_public_key_wrapper(const void *dht)
{
    return dht_get_self_public_key((const DHT *)dht);
}

static const uint8_t *auto_test_nc_dht_get_self_secret_key_wrapper(const void *dht)
{
    return dht_get_self_secret_key((const DHT *)dht);
}

const Net_Crypto_DHT_Funcs auto_test_dht_funcs = {
    auto_test_nc_dht_get_shared_key_sent_wrapper,
    auto_test_nc_dht_get_self_public_key_wrapper,
    auto_test_nc_dht_get_self_secret_key_wrapper,
};

#ifndef USE_IPV6
#define USE_IPV6 1
#endif

// List of live bootstrap nodes. These nodes should have TCP server enabled.
static const struct BootstrapNodes {
    const char   *ip;
    uint16_t      port;
    const uint8_t key[32];
} bootstrap_nodes[] = {
    {
        "144.217.167.73", 33445,
        0x7E, 0x56, 0x68, 0xE0, 0xEE, 0x09, 0xE1, 0x9F,
        0x32, 0x0A, 0xD4, 0x79, 0x02, 0x41, 0x93, 0x31,
        0xFF, 0xEE, 0x14, 0x7B, 0xB3, 0x60, 0x67, 0x69,
        0xCF, 0xBE, 0x92, 0x1A, 0x2A, 0x2F, 0xD3, 0x4C,
    },
    {
        "205.185.115.131", 33445,
        0x30, 0x91, 0xC6, 0xBE, 0xB2, 0xA9, 0x93, 0xF1,
        0xC6, 0x30, 0x0C, 0x16, 0x54, 0x9F, 0xAB, 0xA6,
        0x70, 0x98, 0xFF, 0x3D, 0x62, 0xC6, 0xD2, 0x53,
        0x82, 0x8B, 0x53, 0x14, 0x70, 0xB5, 0x3D, 0x68,
    },
    {
        "tox1.mf-net.eu", 33445,
        0xB3, 0xE5, 0xFA, 0x80, 0xDC, 0x8E, 0xBD, 0x11,
        0x49, 0xAD, 0x2A, 0xB3, 0x5E, 0xD8, 0xB8, 0x5B,
        0xD5, 0x46, 0xDE, 0xDE, 0x26, 0x1C, 0xA5, 0x93,
        0x23, 0x4C, 0x61, 0x92, 0x49, 0x41, 0x95, 0x06,
    },
    {
        "3.0.24.15", 33445,
        0xE2, 0x0A, 0xBC, 0xF3, 0x8C, 0xDB, 0xFF, 0xD7,
        0xD0, 0x4B, 0x29, 0xC9, 0x56, 0xB3, 0x3F, 0x7B,
        0x27, 0xA3, 0xBB, 0x7A, 0xF0, 0x61, 0x81, 0x01,
        0x61, 0x7B, 0x03, 0x6E, 0x4A, 0xEA, 0x40, 0x2D,
    },
    {
        "139.162.110.188", 33445,
        0xF7, 0x6A, 0x11, 0x28, 0x45, 0x47, 0x16, 0x38,
        0x89, 0xDD, 0xC8, 0x9A, 0x77, 0x38, 0xCF, 0x27,
        0x17, 0x97, 0xBF, 0x5E, 0x5E, 0x22, 0x06, 0x43,
        0xE9, 0x7A, 0xD3, 0xC7, 0xE7, 0x90, 0x3D, 0x55,
    },
    {
        "172.105.109.31", 33445,
        0xD4, 0x6E, 0x97, 0xCF, 0x99, 0x5D, 0xC1, 0x82,
        0x0B, 0x92, 0xB7, 0xD8, 0x99, 0xE1, 0x52, 0xA2,
        0x17, 0xD3, 0x6A, 0xBE, 0x22, 0x73, 0x0F, 0xEA,
        0x4B, 0x6B, 0xF1, 0xBF, 0xC0, 0x6C, 0x61, 0x7C,
    },
    { nullptr, 0, 0 },
};

void bootstrap_tox_live_network(Tox *tox, bool enable_tcp)
{
    ck_assert(tox != nullptr);

    for (size_t j = 0; bootstrap_nodes[j].ip != nullptr; ++j) {
        const char *ip = bootstrap_nodes[j].ip;
        uint16_t port = bootstrap_nodes[j].port;
        const uint8_t *key = bootstrap_nodes[j].key;

        Tox_Err_Bootstrap err;
        tox_bootstrap(tox, ip, port, key, &err);

        if (err != TOX_ERR_BOOTSTRAP_OK) {
            fprintf(stderr, "Failed to bootstrap node %zu (%s): error %u\n", j, ip, err);
        }

        if (enable_tcp) {
            tox_add_tcp_relay(tox, ip, port, key, &err);

            if (err != TOX_ERR_BOOTSTRAP_OK) {
                fprintf(stderr, "Failed to add TCP relay %zu (%s): error %u\n", j, ip, err);
            }
        }
    }
}

typedef void autotox_test_cb(AutoTox *autotoxes);

static const char *tox_log_level_name(Tox_Log_Level level)
{
    switch (level) {
        case TOX_LOG_LEVEL_TRACE:
            return "TRACE";

        case TOX_LOG_LEVEL_DEBUG:
            return "DEBUG";

        case TOX_LOG_LEVEL_INFO:
            return "INFO";

        case TOX_LOG_LEVEL_WARNING:
            return "WARNING";

        case TOX_LOG_LEVEL_ERROR:
            return "ERROR";
    }

    return "<unknown>";
}

void print_debug_log(Tox *m, Tox_Log_Level level, const char *file, uint32_t line, const char *func,
                     const char *message, void *user_data)
{
    if (level == TOX_LOG_LEVEL_TRACE) {
        return;
    }

    const uint32_t index = user_data ? *(uint32_t *)user_data : 0;
    fprintf(stderr, "[#%u] %s %s:%u\t%s:\t%s\n", index, tox_log_level_name(level), file, line, func, message);

    if (level == TOX_LOG_LEVEL_ERROR && ABORT_ON_LOG_ERROR) {
        fputs("Aborting test program\n", stderr);
        abort();
    }
}

void print_debug_logger(void *context, Logger_Level level, const char *file, uint32_t line, const char *func, const char *message, void *userdata)
{
    print_debug_log(nullptr, (Tox_Log_Level) level, file, line, func, message, userdata);
}

Tox *tox_new_log_lan(struct Tox_Options *options, Tox_Err_New *err, void *log_user_data, bool lan_discovery)
{
    struct Tox_Options *log_options = options;

    if (log_options == nullptr) {
        log_options = tox_options_new(nullptr);
    }

    assert(log_options != nullptr);

    tox_options_set_ipv6_enabled(log_options, USE_IPV6);
    tox_options_set_local_discovery_enabled(log_options, lan_discovery);
    // Use a higher start port for non-LAN-discovery tests so it's more likely for the LAN discovery
    // test to get the default port 33445.
    const uint16_t start_port = lan_discovery ? 33445 : 33545;
    tox_options_set_start_port(log_options, start_port);
    tox_options_set_end_port(log_options, start_port + 2000);
    tox_options_set_log_callback(log_options, &print_debug_log);
    tox_options_set_log_user_data(log_options, log_user_data);
    Tox *tox = tox_new(log_options, err);

    if (options == nullptr) {
        tox_options_free(log_options);
    }

    return tox;
}

Tox *tox_new_log(struct Tox_Options *options, Tox_Err_New *err, void *log_user_data)
{
    return tox_new_log_lan(options, err, log_user_data, false);
}
