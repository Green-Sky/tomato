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


struct Tox_Event_Group_Privacy_State {
    uint32_t group_number;
    Tox_Group_Privacy_State privacy_state;
};

non_null()
static void tox_event_group_privacy_state_set_group_number(Tox_Event_Group_Privacy_State *group_privacy_state,
        uint32_t group_number)
{
    assert(group_privacy_state != nullptr);
    group_privacy_state->group_number = group_number;
}
uint32_t tox_event_group_privacy_state_get_group_number(const Tox_Event_Group_Privacy_State *group_privacy_state)
{
    assert(group_privacy_state != nullptr);
    return group_privacy_state->group_number;
}

non_null()
static void tox_event_group_privacy_state_set_privacy_state(Tox_Event_Group_Privacy_State *group_privacy_state,
        Tox_Group_Privacy_State privacy_state)
{
    assert(group_privacy_state != nullptr);
    group_privacy_state->privacy_state = privacy_state;
}
Tox_Group_Privacy_State tox_event_group_privacy_state_get_privacy_state(const Tox_Event_Group_Privacy_State *group_privacy_state)
{
    assert(group_privacy_state != nullptr);
    return group_privacy_state->privacy_state;
}

non_null()
static void tox_event_group_privacy_state_construct(Tox_Event_Group_Privacy_State *group_privacy_state)
{
    *group_privacy_state = (Tox_Event_Group_Privacy_State) {
        0
    };
}
non_null()
static void tox_event_group_privacy_state_destruct(Tox_Event_Group_Privacy_State *group_privacy_state, const Memory *mem)
{
    return;
}

bool tox_event_group_privacy_state_pack(
    const Tox_Event_Group_Privacy_State *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_GROUP_PRIVACY_STATE)
           && bin_pack_array(bp, 2)
           && bin_pack_u32(bp, event->group_number)
           && bin_pack_u32(bp, event->privacy_state);
}

non_null()
static bool tox_event_group_privacy_state_unpack_into(
    Tox_Event_Group_Privacy_State *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 2, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->group_number)
           && tox_group_privacy_state_unpack(bu, &event->privacy_state);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Group_Privacy_State *tox_event_get_group_privacy_state(const Tox_Event *event)
{
    return event->type == TOX_EVENT_GROUP_PRIVACY_STATE ? event->data.group_privacy_state : nullptr;
}

Tox_Event_Group_Privacy_State *tox_event_group_privacy_state_new(const Memory *mem)
{
    Tox_Event_Group_Privacy_State *const group_privacy_state =
        (Tox_Event_Group_Privacy_State *)mem_alloc(mem, sizeof(Tox_Event_Group_Privacy_State));

    if (group_privacy_state == nullptr) {
        return nullptr;
    }

    tox_event_group_privacy_state_construct(group_privacy_state);
    return group_privacy_state;
}

void tox_event_group_privacy_state_free(Tox_Event_Group_Privacy_State *group_privacy_state, const Memory *mem)
{
    if (group_privacy_state != nullptr) {
        tox_event_group_privacy_state_destruct(group_privacy_state, mem);
    }
    mem_delete(mem, group_privacy_state);
}

non_null()
static Tox_Event_Group_Privacy_State *tox_events_add_group_privacy_state(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Group_Privacy_State *const group_privacy_state = tox_event_group_privacy_state_new(mem);

    if (group_privacy_state == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_GROUP_PRIVACY_STATE;
    event.data.group_privacy_state = group_privacy_state;

    tox_events_add(events, &event);
    return group_privacy_state;
}

const Tox_Event_Group_Privacy_State *tox_events_get_group_privacy_state(const Tox_Events *events, uint32_t index)
{
    uint32_t group_privacy_state_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (group_privacy_state_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_GROUP_PRIVACY_STATE) {
            const Tox_Event_Group_Privacy_State *group_privacy_state = events->events[i].data.group_privacy_state;
            if (group_privacy_state_index == index) {
                return group_privacy_state;
            }
            ++group_privacy_state_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_group_privacy_state_size(const Tox_Events *events)
{
    uint32_t group_privacy_state_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_GROUP_PRIVACY_STATE) {
            ++group_privacy_state_size;
        }
    }

    return group_privacy_state_size;
}

bool tox_event_group_privacy_state_unpack(
    Tox_Event_Group_Privacy_State **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_group_privacy_state_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_group_privacy_state_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Group_Privacy_State *tox_event_group_privacy_state_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Group_Privacy_State *group_privacy_state = tox_events_add_group_privacy_state(state->events, state->mem);

    if (group_privacy_state == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return group_privacy_state;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_group_privacy_state(Tox *tox, uint32_t group_number, Tox_Group_Privacy_State privacy_state,
        void *user_data)
{
    Tox_Event_Group_Privacy_State *group_privacy_state = tox_event_group_privacy_state_alloc(user_data);

    if (group_privacy_state == nullptr) {
        return;
    }

    tox_event_group_privacy_state_set_group_number(group_privacy_state, group_number);
    tox_event_group_privacy_state_set_privacy_state(group_privacy_state, privacy_state);
}
