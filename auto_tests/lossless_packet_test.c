/* Tests that we can send lossless packets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../testing/misc_tools.h"
#include "../toxcore/util.h"
#include "check_compat.h"

typedef struct State {
    bool custom_packet_received;
} State;

#include "auto_test_support.h"

#define LOSSLESS_PACKET_FILLER 160

static void handle_lossless_packet(Tox *tox, uint32_t friend_number, const uint8_t *data, size_t length, void *user_data)
{
    uint8_t *cmp_packet = (uint8_t *)malloc(tox_max_custom_packet_size());
    ck_assert(cmp_packet != nullptr);
    memset(cmp_packet, LOSSLESS_PACKET_FILLER, tox_max_custom_packet_size());

    if (length == tox_max_custom_packet_size() && memcmp(data, cmp_packet, tox_max_custom_packet_size()) == 0) {
        const AutoTox *autotox = (AutoTox *)user_data;
        State *state = (State *)autotox->state;
        state->custom_packet_received = true;
    }

    free(cmp_packet);
}

static void test_lossless_packet(AutoTox *autotoxes)
{
    tox_callback_friend_lossless_packet(autotoxes[1].tox, &handle_lossless_packet);
    const size_t packet_size = tox_max_custom_packet_size() + 1;
    uint8_t *packet = (uint8_t *)malloc(packet_size);
    ck_assert(packet != nullptr);
    memset(packet, LOSSLESS_PACKET_FILLER, packet_size);

    bool ret = tox_friend_send_lossless_packet(autotoxes[0].tox, 0, packet, packet_size, nullptr);
    ck_assert_msg(ret == false, "should not be able to send custom packets this big %i", ret);

    ret = tox_friend_send_lossless_packet(autotoxes[0].tox, 0, packet, tox_max_custom_packet_size(), nullptr);
    ck_assert_msg(ret == true, "tox_friend_send_lossless_packet fail %i", ret);

    free(packet);

    do {
        iterate_all_wait(autotoxes, 2, ITERATION_INTERVAL);
    } while (!((State *)autotoxes[1].state)->custom_packet_received);
}

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    Run_Auto_Options options = default_run_auto_options();
    options.graph = GRAPH_LINEAR;

    run_auto_test(nullptr, 2, test_lossless_packet, sizeof(State), &options);

    return 0;
}
