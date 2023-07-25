/* Auto Tests: Save and load friends.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../testing/misc_tools.h"
#include "../toxcore/ccompat.h"
#include "../toxcore/crypto_core.h"
#include "../toxcore/tox.h"
#include "auto_test_support.h"
#include "check_compat.h"

struct test_data {
    uint8_t *name;
    uint8_t *status_message;
    bool received_name;
    bool received_status_message;
};

static void set_random(Tox *m, const Random *rng, bool (*setter)(Tox *, const uint8_t *, size_t, Tox_Err_Set_Info *), size_t length)
{
    VLA(uint8_t, text, length);
    uint32_t i;

    for (i = 0; i < length; ++i) {
        text[i] = random_u08(rng);
    }

    setter(m, text, SIZEOF_VLA(text), nullptr);
}

static void alloc_string(uint8_t **to, size_t length)
{
    free(*to);
    *to = (uint8_t *)malloc(length);
    ck_assert(*to != nullptr);
}

static void set_string(uint8_t **to, const uint8_t *from, size_t length)
{
    alloc_string(to, length);
    memcpy(*to, from, length);
}

static void namechange_callback(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data)
{
    struct test_data *to_compare = (struct test_data *)user_data;
    set_string(&to_compare->name, name, length);
    to_compare->received_name = true;
}

static void statuschange_callback(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length,
                                  void *user_data)
{
    struct test_data *to_compare = (struct test_data *)user_data;
    set_string(&to_compare->status_message, message, length);
    to_compare->received_status_message = true;
}

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    Tox *const tox1 = tox_new_log(nullptr, nullptr, nullptr);
    Tox *const tox2 = tox_new_log(nullptr, nullptr, nullptr);

    printf("bootstrapping tox2 off tox1\n");
    uint8_t dht_key[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_dht_id(tox1, dht_key);
    const uint16_t dht_port = tox_self_get_udp_port(tox1, nullptr);

    tox_bootstrap(tox2, "localhost", dht_port, dht_key, nullptr);

    struct test_data to_compare = {nullptr};

    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    tox_self_get_public_key(tox1, public_key);
    tox_friend_add_norequest(tox2, public_key, nullptr);
    tox_self_get_public_key(tox2, public_key);
    tox_friend_add_norequest(tox1, public_key, nullptr);

    uint8_t *reference_name = (uint8_t *)malloc(tox_max_name_length());
    uint8_t *reference_status = (uint8_t *)malloc(tox_max_status_message_length());
    ck_assert(reference_name != nullptr);
    ck_assert(reference_status != nullptr);

    const Random *rng = system_random();
    ck_assert(rng != nullptr);
    set_random(tox1, rng, tox_self_set_name, tox_max_name_length());
    set_random(tox2, rng, tox_self_set_name, tox_max_name_length());
    set_random(tox1, rng, tox_self_set_status_message, tox_max_status_message_length());
    set_random(tox2, rng, tox_self_set_status_message, tox_max_status_message_length());

    tox_self_get_name(tox2, reference_name);
    tox_self_get_status_message(tox2, reference_status);

    tox_callback_friend_name(tox1, namechange_callback);
    tox_callback_friend_status_message(tox1, statuschange_callback);

    while (true) {
        if (tox_self_get_connection_status(tox1) &&
                tox_self_get_connection_status(tox2) &&
                tox_friend_get_connection_status(tox1, 0, nullptr) == TOX_CONNECTION_UDP) {
            printf("Connected.\n");
            break;
        }

        tox_iterate(tox1, &to_compare);
        tox_iterate(tox2, nullptr);

        c_sleep(tox_iteration_interval(tox1));
    }

    while (true) {
        if (to_compare.received_name && to_compare.received_status_message) {
            printf("Exchanged names and status messages.\n");
            break;
        }

        tox_iterate(tox1, &to_compare);
        tox_iterate(tox2, nullptr);

        c_sleep(tox_iteration_interval(tox1));
    }

    size_t save_size = tox_get_savedata_size(tox1);
    uint8_t *savedata = (uint8_t *)malloc(save_size);
    tox_get_savedata(tox1, savedata);

    struct Tox_Options *const options = tox_options_new(nullptr);
    tox_options_set_savedata_type(options, TOX_SAVEDATA_TYPE_TOX_SAVE);
    tox_options_set_savedata_data(options, savedata, save_size);

    Tox *const tox_to_compare = tox_new_log(options, nullptr, nullptr);

    alloc_string(&to_compare.name, tox_friend_get_name_size(tox_to_compare, 0, nullptr));
    tox_friend_get_name(tox_to_compare, 0, to_compare.name, nullptr);
    alloc_string(&to_compare.status_message, tox_friend_get_status_message_size(tox_to_compare, 0, nullptr));
    tox_friend_get_status_message(tox_to_compare, 0, to_compare.status_message, nullptr);

    ck_assert_msg(memcmp(reference_name, to_compare.name, tox_max_name_length()) == 0,
                  "incorrect name: should be all zeroes");
    ck_assert_msg(memcmp(reference_status, to_compare.status_message, tox_max_status_message_length()) == 0,
                  "incorrect status message: should be all zeroes");

    tox_options_free(options);
    tox_kill(tox1);
    tox_kill(tox2);
    tox_kill(tox_to_compare);
    free(savedata);
    free(to_compare.name);
    free(to_compare.status_message);
    free(reference_status);
    free(reference_name);

    return 0;
}
