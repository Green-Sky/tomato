/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2023 The TokTok team.
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
#include "../tox_unpack.h"


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Group_Custom_Packet {
    uint32_t group_number;
    uint32_t peer_id;
    uint8_t *data;
    uint32_t data_length;
};

non_null()
static void tox_event_group_custom_packet_set_group_number(Tox_Event_Group_Custom_Packet *group_custom_packet,
        uint32_t group_number)
{
    assert(group_custom_packet != nullptr);
    group_custom_packet->group_number = group_number;
}
uint32_t tox_event_group_custom_packet_get_group_number(const Tox_Event_Group_Custom_Packet *group_custom_packet)
{
    assert(group_custom_packet != nullptr);
    return group_custom_packet->group_number;
}

non_null()
static void tox_event_group_custom_packet_set_peer_id(Tox_Event_Group_Custom_Packet *group_custom_packet,
        uint32_t peer_id)
{
    assert(group_custom_packet != nullptr);
    group_custom_packet->peer_id = peer_id;
}
uint32_t tox_event_group_custom_packet_get_peer_id(const Tox_Event_Group_Custom_Packet *group_custom_packet)
{
    assert(group_custom_packet != nullptr);
    return group_custom_packet->peer_id;
}

non_null()
static bool tox_event_group_custom_packet_set_data(Tox_Event_Group_Custom_Packet *group_custom_packet,
        const uint8_t *data, uint32_t data_length)
{
    assert(group_custom_packet != nullptr);

    if (group_custom_packet->data != nullptr) {
        free(group_custom_packet->data);
        group_custom_packet->data = nullptr;
        group_custom_packet->data_length = 0;
    }

    group_custom_packet->data = (uint8_t *)malloc(data_length);

    if (group_custom_packet->data == nullptr) {
        return false;
    }

    memcpy(group_custom_packet->data, data, data_length);
    group_custom_packet->data_length = data_length;
    return true;
}
uint32_t tox_event_group_custom_packet_get_data_length(const Tox_Event_Group_Custom_Packet *group_custom_packet)
{
    assert(group_custom_packet != nullptr);
    return group_custom_packet->data_length;
}
const uint8_t *tox_event_group_custom_packet_get_data(const Tox_Event_Group_Custom_Packet *group_custom_packet)
{
    assert(group_custom_packet != nullptr);
    return group_custom_packet->data;
}

non_null()
static void tox_event_group_custom_packet_construct(Tox_Event_Group_Custom_Packet *group_custom_packet)
{
    *group_custom_packet = (Tox_Event_Group_Custom_Packet) {
        0
    };
}
non_null()
static void tox_event_group_custom_packet_destruct(Tox_Event_Group_Custom_Packet *group_custom_packet, const Memory *mem)
{
    free(group_custom_packet->data);
}

bool tox_event_group_custom_packet_pack(
    const Tox_Event_Group_Custom_Packet *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_GROUP_CUSTOM_PACKET)
           && bin_pack_array(bp, 3)
           && bin_pack_u32(bp, event->group_number)
           && bin_pack_u32(bp, event->peer_id)
           && bin_pack_bin(bp, event->data, event->data_length);
}

non_null()
static bool tox_event_group_custom_packet_unpack_into(
    Tox_Event_Group_Custom_Packet *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 3, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->group_number)
           && bin_unpack_u32(bu, &event->peer_id)
           && bin_unpack_bin(bu, &event->data, &event->data_length);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Group_Custom_Packet *tox_event_get_group_custom_packet(const Tox_Event *event)
{
    return event->type == TOX_EVENT_GROUP_CUSTOM_PACKET ? event->data.group_custom_packet : nullptr;
}

Tox_Event_Group_Custom_Packet *tox_event_group_custom_packet_new(const Memory *mem)
{
    Tox_Event_Group_Custom_Packet *const group_custom_packet =
        (Tox_Event_Group_Custom_Packet *)mem_alloc(mem, sizeof(Tox_Event_Group_Custom_Packet));

    if (group_custom_packet == nullptr) {
        return nullptr;
    }

    tox_event_group_custom_packet_construct(group_custom_packet);
    return group_custom_packet;
}

void tox_event_group_custom_packet_free(Tox_Event_Group_Custom_Packet *group_custom_packet, const Memory *mem)
{
    if (group_custom_packet != nullptr) {
        tox_event_group_custom_packet_destruct(group_custom_packet, mem);
    }
    mem_delete(mem, group_custom_packet);
}

non_null()
static Tox_Event_Group_Custom_Packet *tox_events_add_group_custom_packet(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Group_Custom_Packet *const group_custom_packet = tox_event_group_custom_packet_new(mem);

    if (group_custom_packet == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_GROUP_CUSTOM_PACKET;
    event.data.group_custom_packet = group_custom_packet;

    tox_events_add(events, &event);
    return group_custom_packet;
}

const Tox_Event_Group_Custom_Packet *tox_events_get_group_custom_packet(const Tox_Events *events, uint32_t index)
{
    uint32_t group_custom_packet_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (group_custom_packet_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_GROUP_CUSTOM_PACKET) {
            const Tox_Event_Group_Custom_Packet *group_custom_packet = events->events[i].data.group_custom_packet;
            if (group_custom_packet_index == index) {
                return group_custom_packet;
            }
            ++group_custom_packet_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_group_custom_packet_size(const Tox_Events *events)
{
    uint32_t group_custom_packet_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_GROUP_CUSTOM_PACKET) {
            ++group_custom_packet_size;
        }
    }

    return group_custom_packet_size;
}

bool tox_event_group_custom_packet_unpack(
    Tox_Event_Group_Custom_Packet **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_group_custom_packet_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_group_custom_packet_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Group_Custom_Packet *tox_event_group_custom_packet_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Group_Custom_Packet *group_custom_packet = tox_events_add_group_custom_packet(state->events, state->mem);

    if (group_custom_packet == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return group_custom_packet;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_group_custom_packet(Tox *tox, uint32_t group_number, uint32_t peer_id, const uint8_t *data, size_t length,
        void *user_data)
{
    Tox_Event_Group_Custom_Packet *group_custom_packet = tox_event_group_custom_packet_alloc(user_data);

    if (group_custom_packet == nullptr) {
        return;
    }

    tox_event_group_custom_packet_set_group_number(group_custom_packet, group_number);
    tox_event_group_custom_packet_set_peer_id(group_custom_packet, peer_id);
    tox_event_group_custom_packet_set_data(group_custom_packet, data, length);
}
