/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022 The TokTok team.
 */

#include "events_alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../bin_pack.h"
#include "../bin_unpack.h"
#include "../ccompat.h"
#include "../tox.h"
#include "../tox_events.h"


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Friend_Lossy_Packet {
    uint32_t friend_number;
    uint8_t *data;
    uint32_t data_length;
};

non_null()
static void tox_event_friend_lossy_packet_construct(Tox_Event_Friend_Lossy_Packet *friend_lossy_packet)
{
    *friend_lossy_packet = (Tox_Event_Friend_Lossy_Packet) {
        0
    };
}
non_null()
static void tox_event_friend_lossy_packet_destruct(Tox_Event_Friend_Lossy_Packet *friend_lossy_packet)
{
    free(friend_lossy_packet->data);
}

non_null()
static void tox_event_friend_lossy_packet_set_friend_number(Tox_Event_Friend_Lossy_Packet *friend_lossy_packet,
        uint32_t friend_number)
{
    assert(friend_lossy_packet != nullptr);
    friend_lossy_packet->friend_number = friend_number;
}
uint32_t tox_event_friend_lossy_packet_get_friend_number(const Tox_Event_Friend_Lossy_Packet *friend_lossy_packet)
{
    assert(friend_lossy_packet != nullptr);
    return friend_lossy_packet->friend_number;
}

non_null()
static bool tox_event_friend_lossy_packet_set_data(Tox_Event_Friend_Lossy_Packet *friend_lossy_packet,
        const uint8_t *data, uint32_t data_length)
{
    assert(friend_lossy_packet != nullptr);

    if (friend_lossy_packet->data != nullptr) {
        free(friend_lossy_packet->data);
        friend_lossy_packet->data = nullptr;
        friend_lossy_packet->data_length = 0;
    }

    friend_lossy_packet->data = (uint8_t *)malloc(data_length);

    if (friend_lossy_packet->data == nullptr) {
        return false;
    }

    memcpy(friend_lossy_packet->data, data, data_length);
    friend_lossy_packet->data_length = data_length;
    return true;
}
uint32_t tox_event_friend_lossy_packet_get_data_length(const Tox_Event_Friend_Lossy_Packet *friend_lossy_packet)
{
    assert(friend_lossy_packet != nullptr);
    return friend_lossy_packet->data_length;
}
const uint8_t *tox_event_friend_lossy_packet_get_data(const Tox_Event_Friend_Lossy_Packet *friend_lossy_packet)
{
    assert(friend_lossy_packet != nullptr);
    return friend_lossy_packet->data;
}

non_null()
static bool tox_event_friend_lossy_packet_pack(
    const Tox_Event_Friend_Lossy_Packet *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_FRIEND_LOSSY_PACKET)
           && bin_pack_array(bp, 2)
           && bin_pack_u32(bp, event->friend_number)
           && bin_pack_bin(bp, event->data, event->data_length);
}

non_null()
static bool tox_event_friend_lossy_packet_unpack(
    Tox_Event_Friend_Lossy_Packet *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 2)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->friend_number)
           && bin_unpack_bin(bu, &event->data, &event->data_length);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Friend_Lossy_Packet *tox_events_add_friend_lossy_packet(Tox_Events *events)
{
    if (events->friend_lossy_packet_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->friend_lossy_packet_size == events->friend_lossy_packet_capacity) {
        const uint32_t new_friend_lossy_packet_capacity = events->friend_lossy_packet_capacity * 2 + 1;
        Tox_Event_Friend_Lossy_Packet *new_friend_lossy_packet = (Tox_Event_Friend_Lossy_Packet *)realloc(
                    events->friend_lossy_packet, new_friend_lossy_packet_capacity * sizeof(Tox_Event_Friend_Lossy_Packet));

        if (new_friend_lossy_packet == nullptr) {
            return nullptr;
        }

        events->friend_lossy_packet = new_friend_lossy_packet;
        events->friend_lossy_packet_capacity = new_friend_lossy_packet_capacity;
    }

    Tox_Event_Friend_Lossy_Packet *const friend_lossy_packet =
        &events->friend_lossy_packet[events->friend_lossy_packet_size];
    tox_event_friend_lossy_packet_construct(friend_lossy_packet);
    ++events->friend_lossy_packet_size;
    return friend_lossy_packet;
}

void tox_events_clear_friend_lossy_packet(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->friend_lossy_packet_size; ++i) {
        tox_event_friend_lossy_packet_destruct(&events->friend_lossy_packet[i]);
    }

    free(events->friend_lossy_packet);
    events->friend_lossy_packet = nullptr;
    events->friend_lossy_packet_size = 0;
    events->friend_lossy_packet_capacity = 0;
}

uint32_t tox_events_get_friend_lossy_packet_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->friend_lossy_packet_size;
}

const Tox_Event_Friend_Lossy_Packet *tox_events_get_friend_lossy_packet(const Tox_Events *events, uint32_t index)
{
    assert(index < events->friend_lossy_packet_size);
    assert(events->friend_lossy_packet != nullptr);
    return &events->friend_lossy_packet[index];
}

bool tox_events_pack_friend_lossy_packet(const Tox_Events *events, Bin_Pack *bp)
{
    const uint32_t size = tox_events_get_friend_lossy_packet_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (!tox_event_friend_lossy_packet_pack(tox_events_get_friend_lossy_packet(events, i), bp)) {
            return false;
        }
    }
    return true;
}

bool tox_events_unpack_friend_lossy_packet(Tox_Events *events, Bin_Unpack *bu)
{
    Tox_Event_Friend_Lossy_Packet *event = tox_events_add_friend_lossy_packet(events);

    if (event == nullptr) {
        return false;
    }

    return tox_event_friend_lossy_packet_unpack(event, bu);
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_friend_lossy_packet(Tox *tox, uint32_t friend_number, const uint8_t *data, size_t length,
        void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return;
    }

    Tox_Event_Friend_Lossy_Packet *friend_lossy_packet = tox_events_add_friend_lossy_packet(state->events);

    if (friend_lossy_packet == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_friend_lossy_packet_set_friend_number(friend_lossy_packet, friend_number);
    tox_event_friend_lossy_packet_set_data(friend_lossy_packet, data, length);
}
