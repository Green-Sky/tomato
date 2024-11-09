/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2013-2015 Tox project.
 */
#include "bwcontroller.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ring_buffer.h"
#include "toxav_hacks.h"

#include "../toxcore/ccompat.h"
#include "../toxcore/logger.h"
#include "../toxcore/mono_time.h"
#include "../toxcore/network.h"
#include "../toxcore/tox_private.h"
#include "../toxcore/util.h"


#define BWC_PACKET_ID 196
#define BWC_SEND_INTERVAL_MS 950     // 0.95s
#define BWC_AVG_PKT_COUNT 20
#define BWC_AVG_LOSS_OVER_CYCLES_COUNT 30

typedef struct BWCCycle {
    uint32_t last_recv_timestamp; /* Last recv update time stamp */
    uint32_t last_sent_timestamp; /* Last sent update time stamp */
    uint32_t last_refresh_timestamp; /* Last refresh time stamp */

    uint32_t lost;
    uint32_t recv;
} BWCCycle;

typedef struct BWCRcvPkt {
    uint32_t packet_length_array[BWC_AVG_PKT_COUNT];
    RingBuffer *rb;
} BWCRcvPkt;

struct BWController {
    m_cb *mcb;
    void *mcb_user_data;
    Tox *tox;
    const Logger *log;
    uint32_t friend_number;

    BWCCycle cycle;

    BWCRcvPkt rcvpkt; /* To calculate average received packet (this means split parts, not the full message!) */

    uint32_t packet_loss_counted_cycles;
    Mono_Time *bwc_mono_time;
    bool bwc_receive_active; /* if this is set to false then incoming bwc packets will not be processed by bwc_handle_data() */
};

struct BWCMessage {
    uint32_t lost;
    uint32_t recv;
};

static void bwc_handle_data(Tox *tox, uint32_t friend_number, const uint8_t *data, size_t length, void *user_data);
static void send_update(BWController *bwc);


BWController *bwc_new(const Logger *log, Tox *tox, uint32_t friendnumber, m_cb *mcb, void *mcb_user_data,
                      Mono_Time *bwc_mono_time)
{
    BWController *retu = (BWController *)calloc(1, sizeof(BWController));

    if (retu == nullptr) {
        return nullptr;
    }

    LOGGER_DEBUG(log, "Creating bandwidth controller");

    retu->mcb = mcb;
    retu->mcb_user_data = mcb_user_data;
    retu->friend_number = friendnumber;
    retu->bwc_mono_time = bwc_mono_time;
    const uint64_t now = current_time_monotonic(bwc_mono_time);
    retu->cycle.last_sent_timestamp = now;
    retu->cycle.last_refresh_timestamp = now;
    retu->tox = tox;
    retu->log = log;
    retu->bwc_receive_active = true;
    retu->rcvpkt.rb = rb_new(BWC_AVG_PKT_COUNT);
    retu->cycle.lost = 0;
    retu->cycle.recv = 0;
    retu->packet_loss_counted_cycles = 0;

    /* Fill with zeros */
    for (int i = 0; i < BWC_AVG_PKT_COUNT; ++i) {
        rb_write(retu->rcvpkt.rb, &retu->rcvpkt.packet_length_array[i]);
    }

    return retu;
}

void bwc_kill(BWController *bwc)
{
    if (bwc == nullptr) {
        return;
    }

    rb_kill(bwc->rcvpkt.rb);
    free(bwc);
}

void bwc_add_lost(BWController *bwc, uint32_t bytes_lost)
{
    if (bwc == nullptr) {
        return;
    }

    if (bytes_lost > 0) {
        LOGGER_DEBUG(bwc->log, "BWC lost(1): %d", (int)bytes_lost);
        bwc->cycle.lost += bytes_lost;
        send_update(bwc);
    }
}

void bwc_add_recv(BWController *bwc, uint32_t recv_bytes)
{
    if (bwc == nullptr || recv_bytes == 0) {
        return;
    }

    ++bwc->packet_loss_counted_cycles;
    bwc->cycle.recv += recv_bytes;
    send_update(bwc);
}

static void send_update(BWController *bwc)
{
    if (bwc->packet_loss_counted_cycles > BWC_AVG_LOSS_OVER_CYCLES_COUNT &&
            current_time_monotonic(bwc->bwc_mono_time) - bwc->cycle.last_sent_timestamp > BWC_SEND_INTERVAL_MS) {
        bwc->packet_loss_counted_cycles = 0;

        if (bwc->cycle.lost != 0) {
            LOGGER_DEBUG(bwc->log, "%p Sent update rcv: %u lost: %u percent: %f %%",
                         (void *)bwc, bwc->cycle.recv, bwc->cycle.lost,
                         ((double)bwc->cycle.lost / (bwc->cycle.recv + bwc->cycle.lost)) * 100.0);
            uint8_t bwc_packet[sizeof(struct BWCMessage) + 1];
            size_t offset = 0;

            bwc_packet[offset] = BWC_PACKET_ID; // set packet ID
            ++offset;

            offset += net_pack_u32(bwc_packet + offset, bwc->cycle.lost);
            offset += net_pack_u32(bwc_packet + offset, bwc->cycle.recv);
            assert(offset == sizeof(bwc_packet));

            Tox_Err_Friend_Custom_Packet error;
            tox_friend_send_lossy_packet(bwc->tox, bwc->friend_number, bwc_packet, sizeof(bwc_packet), &error);

            if (error != TOX_ERR_FRIEND_CUSTOM_PACKET_OK) {
                LOGGER_WARNING(bwc->log, "BWC send failed: %d", error);
            }
        }

        bwc->cycle.last_sent_timestamp = current_time_monotonic(bwc->bwc_mono_time);
        bwc->cycle.lost = 0;
        bwc->cycle.recv = 0;
    }
}

static int on_update(BWController *bwc, const struct BWCMessage *msg)
{
    LOGGER_DEBUG(bwc->log, "%p Got update from peer", (void *)bwc);

    /* Peers sent update too soon */
    if (bwc->cycle.last_recv_timestamp + BWC_SEND_INTERVAL_MS > current_time_monotonic(bwc->bwc_mono_time)) {
        LOGGER_INFO(bwc->log, "%p Rejecting extra update", (void *)bwc);
        return -1;
    }

    bwc->cycle.last_recv_timestamp = current_time_monotonic(bwc->bwc_mono_time);

    const uint32_t lost = msg->lost;

    if (lost != 0 && bwc->mcb != nullptr) {
        const uint32_t recv = msg->recv;
        LOGGER_DEBUG(bwc->log, "recved: %u lost: %u percentage: %f %%", recv, lost,
                     ((double) lost / (recv + lost)) * 100.0);
        bwc->mcb(bwc, bwc->friend_number,
                 (float)lost / (recv + lost),
                 bwc->mcb_user_data);
    }

    return 0;
}

static void bwc_handle_data(Tox *tox, uint32_t friend_number, const uint8_t *data, size_t length, void *user_data)
{
    /* get BWController object from Tox and friend number */
    ToxAV *toxav = (ToxAV *)tox_get_av_object(tox);

    if (toxav == nullptr) {
        // LOGGER_ERROR(log, "Could not get ToxAV object from Tox");
        return;
    }

    const Logger *log = toxav_get_logger(toxav);

    if (length - 1 != sizeof(struct BWCMessage)) {
        LOGGER_ERROR(log, "Got BWCMessage of insufficient size.");
        return;
    }

    const ToxAVCall *call = call_get(toxav, friend_number);

    if (call == nullptr) {
        LOGGER_ERROR(log, "Could not get ToxAVCall object from ToxAV.");
        return;
    }

    /* get Call object from Tox and friend number */
    BWController *bwc = bwc_controller_get(call);

    if (bwc == nullptr) {
        LOGGER_WARNING(log, "No session!");
        return;
    }

    if (!bwc->bwc_receive_active) {
        LOGGER_WARNING(log, "receiving not allowed!");
        return;
    }

    size_t offset = 1;  // Ignore packet id.
    struct BWCMessage msg;
    offset += net_unpack_u32(data + offset, &msg.lost);
    offset += net_unpack_u32(data + offset, &msg.recv);
    assert(offset == length);

    on_update(bwc, &msg);
}

void bwc_allow_receiving(Tox *tox)
{
    tox_callback_friend_lossy_packet_per_pktid(tox, bwc_handle_data, BWC_PACKET_ID);
}

void bwc_stop_receiving(Tox *tox)
{
    tox_callback_friend_lossy_packet_per_pktid(tox, nullptr, BWC_PACKET_ID);
}
