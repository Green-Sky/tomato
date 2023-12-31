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


struct Tox_Event_Group_Peer_Exit {
    uint32_t group_number;
    uint32_t peer_id;
    Tox_Group_Exit_Type exit_type;
    uint8_t *name;
    uint32_t name_length;
    uint8_t *part_message;
    uint32_t part_message_length;
};

non_null()
static void tox_event_group_peer_exit_set_group_number(Tox_Event_Group_Peer_Exit *group_peer_exit,
        uint32_t group_number)
{
    assert(group_peer_exit != nullptr);
    group_peer_exit->group_number = group_number;
}
uint32_t tox_event_group_peer_exit_get_group_number(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->group_number;
}

non_null()
static void tox_event_group_peer_exit_set_peer_id(Tox_Event_Group_Peer_Exit *group_peer_exit,
        uint32_t peer_id)
{
    assert(group_peer_exit != nullptr);
    group_peer_exit->peer_id = peer_id;
}
uint32_t tox_event_group_peer_exit_get_peer_id(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->peer_id;
}

non_null()
static void tox_event_group_peer_exit_set_exit_type(Tox_Event_Group_Peer_Exit *group_peer_exit,
        Tox_Group_Exit_Type exit_type)
{
    assert(group_peer_exit != nullptr);
    group_peer_exit->exit_type = exit_type;
}
Tox_Group_Exit_Type tox_event_group_peer_exit_get_exit_type(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->exit_type;
}

non_null()
static bool tox_event_group_peer_exit_set_name(Tox_Event_Group_Peer_Exit *group_peer_exit,
        const uint8_t *name, uint32_t name_length)
{
    assert(group_peer_exit != nullptr);

    if (group_peer_exit->name != nullptr) {
        free(group_peer_exit->name);
        group_peer_exit->name = nullptr;
        group_peer_exit->name_length = 0;
    }

    group_peer_exit->name = (uint8_t *)malloc(name_length);

    if (group_peer_exit->name == nullptr) {
        return false;
    }

    memcpy(group_peer_exit->name, name, name_length);
    group_peer_exit->name_length = name_length;
    return true;
}
uint32_t tox_event_group_peer_exit_get_name_length(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->name_length;
}
const uint8_t *tox_event_group_peer_exit_get_name(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->name;
}

non_null()
static bool tox_event_group_peer_exit_set_part_message(Tox_Event_Group_Peer_Exit *group_peer_exit,
        const uint8_t *part_message, uint32_t part_message_length)
{
    assert(group_peer_exit != nullptr);

    if (group_peer_exit->part_message != nullptr) {
        free(group_peer_exit->part_message);
        group_peer_exit->part_message = nullptr;
        group_peer_exit->part_message_length = 0;
    }

    group_peer_exit->part_message = (uint8_t *)malloc(part_message_length);

    if (group_peer_exit->part_message == nullptr) {
        return false;
    }

    memcpy(group_peer_exit->part_message, part_message, part_message_length);
    group_peer_exit->part_message_length = part_message_length;
    return true;
}
uint32_t tox_event_group_peer_exit_get_part_message_length(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->part_message_length;
}
const uint8_t *tox_event_group_peer_exit_get_part_message(const Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    assert(group_peer_exit != nullptr);
    return group_peer_exit->part_message;
}

non_null()
static void tox_event_group_peer_exit_construct(Tox_Event_Group_Peer_Exit *group_peer_exit)
{
    *group_peer_exit = (Tox_Event_Group_Peer_Exit) {
        0
    };
}
non_null()
static void tox_event_group_peer_exit_destruct(Tox_Event_Group_Peer_Exit *group_peer_exit, const Memory *mem)
{
    free(group_peer_exit->name);
    free(group_peer_exit->part_message);
}

bool tox_event_group_peer_exit_pack(
    const Tox_Event_Group_Peer_Exit *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_GROUP_PEER_EXIT)
           && bin_pack_array(bp, 5)
           && bin_pack_u32(bp, event->group_number)
           && bin_pack_u32(bp, event->peer_id)
           && bin_pack_u32(bp, event->exit_type)
           && bin_pack_bin(bp, event->name, event->name_length)
           && bin_pack_bin(bp, event->part_message, event->part_message_length);
}

non_null()
static bool tox_event_group_peer_exit_unpack_into(
    Tox_Event_Group_Peer_Exit *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 5, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->group_number)
           && bin_unpack_u32(bu, &event->peer_id)
           && tox_group_exit_type_unpack(bu, &event->exit_type)
           && bin_unpack_bin(bu, &event->name, &event->name_length)
           && bin_unpack_bin(bu, &event->part_message, &event->part_message_length);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Group_Peer_Exit *tox_event_get_group_peer_exit(const Tox_Event *event)
{
    return event->type == TOX_EVENT_GROUP_PEER_EXIT ? event->data.group_peer_exit : nullptr;
}

Tox_Event_Group_Peer_Exit *tox_event_group_peer_exit_new(const Memory *mem)
{
    Tox_Event_Group_Peer_Exit *const group_peer_exit =
        (Tox_Event_Group_Peer_Exit *)mem_alloc(mem, sizeof(Tox_Event_Group_Peer_Exit));

    if (group_peer_exit == nullptr) {
        return nullptr;
    }

    tox_event_group_peer_exit_construct(group_peer_exit);
    return group_peer_exit;
}

void tox_event_group_peer_exit_free(Tox_Event_Group_Peer_Exit *group_peer_exit, const Memory *mem)
{
    if (group_peer_exit != nullptr) {
        tox_event_group_peer_exit_destruct(group_peer_exit, mem);
    }
    mem_delete(mem, group_peer_exit);
}

non_null()
static Tox_Event_Group_Peer_Exit *tox_events_add_group_peer_exit(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Group_Peer_Exit *const group_peer_exit = tox_event_group_peer_exit_new(mem);

    if (group_peer_exit == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_GROUP_PEER_EXIT;
    event.data.group_peer_exit = group_peer_exit;

    tox_events_add(events, &event);
    return group_peer_exit;
}

const Tox_Event_Group_Peer_Exit *tox_events_get_group_peer_exit(const Tox_Events *events, uint32_t index)
{
    uint32_t group_peer_exit_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (group_peer_exit_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_GROUP_PEER_EXIT) {
            const Tox_Event_Group_Peer_Exit *group_peer_exit = events->events[i].data.group_peer_exit;
            if (group_peer_exit_index == index) {
                return group_peer_exit;
            }
            ++group_peer_exit_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_group_peer_exit_size(const Tox_Events *events)
{
    uint32_t group_peer_exit_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_GROUP_PEER_EXIT) {
            ++group_peer_exit_size;
        }
    }

    return group_peer_exit_size;
}

bool tox_event_group_peer_exit_unpack(
    Tox_Event_Group_Peer_Exit **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_group_peer_exit_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_group_peer_exit_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Group_Peer_Exit *tox_event_group_peer_exit_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Group_Peer_Exit *group_peer_exit = tox_events_add_group_peer_exit(state->events, state->mem);

    if (group_peer_exit == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return group_peer_exit;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_group_peer_exit(Tox *tox, uint32_t group_number, uint32_t peer_id, Tox_Group_Exit_Type exit_type, const uint8_t *name, size_t name_length, const uint8_t *part_message, size_t part_message_length,
        void *user_data)
{
    Tox_Event_Group_Peer_Exit *group_peer_exit = tox_event_group_peer_exit_alloc(user_data);

    if (group_peer_exit == nullptr) {
        return;
    }

    tox_event_group_peer_exit_set_group_number(group_peer_exit, group_number);
    tox_event_group_peer_exit_set_peer_id(group_peer_exit, peer_id);
    tox_event_group_peer_exit_set_exit_type(group_peer_exit, exit_type);
    tox_event_group_peer_exit_set_name(group_peer_exit, name, name_length);
    tox_event_group_peer_exit_set_part_message(group_peer_exit, part_message, part_message_length);
}
