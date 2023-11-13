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


struct Tox_Event_Group_Topic_Lock {
    uint32_t group_number;
    Tox_Group_Topic_Lock topic_lock;
};

non_null()
static void tox_event_group_topic_lock_construct(Tox_Event_Group_Topic_Lock *group_topic_lock)
{
    *group_topic_lock = (Tox_Event_Group_Topic_Lock) {
        0
    };
}
non_null()
static void tox_event_group_topic_lock_destruct(Tox_Event_Group_Topic_Lock *group_topic_lock)
{
    return;
}

non_null()
static void tox_event_group_topic_lock_set_group_number(Tox_Event_Group_Topic_Lock *group_topic_lock,
        uint32_t group_number)
{
    assert(group_topic_lock != nullptr);
    group_topic_lock->group_number = group_number;
}
uint32_t tox_event_group_topic_lock_get_group_number(const Tox_Event_Group_Topic_Lock *group_topic_lock)
{
    assert(group_topic_lock != nullptr);
    return group_topic_lock->group_number;
}

non_null()
static void tox_event_group_topic_lock_set_topic_lock(Tox_Event_Group_Topic_Lock *group_topic_lock,
        Tox_Group_Topic_Lock topic_lock)
{
    assert(group_topic_lock != nullptr);
    group_topic_lock->topic_lock = topic_lock;
}
Tox_Group_Topic_Lock tox_event_group_topic_lock_get_topic_lock(const Tox_Event_Group_Topic_Lock *group_topic_lock)
{
    assert(group_topic_lock != nullptr);
    return group_topic_lock->topic_lock;
}

non_null()
static bool tox_event_group_topic_lock_pack(
    const Tox_Event_Group_Topic_Lock *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_GROUP_TOPIC_LOCK)
           && bin_pack_array(bp, 2)
           && bin_pack_u32(bp, event->group_number)
           && bin_pack_u32(bp, event->topic_lock);
}

non_null()
static bool tox_event_group_topic_lock_unpack(
    Tox_Event_Group_Topic_Lock *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 2, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->group_number)
           && tox_unpack_group_topic_lock(bu, &event->topic_lock);
}


/*****************************************************
 *
 * :: add/clear/get
 *
 *****************************************************/


non_null()
static Tox_Event_Group_Topic_Lock *tox_events_add_group_topic_lock(Tox_Events *events)
{
    if (events->group_topic_lock_size == UINT32_MAX) {
        return nullptr;
    }

    if (events->group_topic_lock_size == events->group_topic_lock_capacity) {
        const uint32_t new_group_topic_lock_capacity = events->group_topic_lock_capacity * 2 + 1;
        Tox_Event_Group_Topic_Lock *new_group_topic_lock = (Tox_Event_Group_Topic_Lock *)
                realloc(
                    events->group_topic_lock,
                    new_group_topic_lock_capacity * sizeof(Tox_Event_Group_Topic_Lock));

        if (new_group_topic_lock == nullptr) {
            return nullptr;
        }

        events->group_topic_lock = new_group_topic_lock;
        events->group_topic_lock_capacity = new_group_topic_lock_capacity;
    }

    Tox_Event_Group_Topic_Lock *const group_topic_lock =
        &events->group_topic_lock[events->group_topic_lock_size];
    tox_event_group_topic_lock_construct(group_topic_lock);
    ++events->group_topic_lock_size;
    return group_topic_lock;
}

void tox_events_clear_group_topic_lock(Tox_Events *events)
{
    if (events == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < events->group_topic_lock_size; ++i) {
        tox_event_group_topic_lock_destruct(&events->group_topic_lock[i]);
    }

    free(events->group_topic_lock);
    events->group_topic_lock = nullptr;
    events->group_topic_lock_size = 0;
    events->group_topic_lock_capacity = 0;
}

uint32_t tox_events_get_group_topic_lock_size(const Tox_Events *events)
{
    if (events == nullptr) {
        return 0;
    }

    return events->group_topic_lock_size;
}

const Tox_Event_Group_Topic_Lock *tox_events_get_group_topic_lock(const Tox_Events *events, uint32_t index)
{
    assert(index < events->group_topic_lock_size);
    assert(events->group_topic_lock != nullptr);
    return &events->group_topic_lock[index];
}

bool tox_events_pack_group_topic_lock(const Tox_Events *events, Bin_Pack *bp)
{
    const uint32_t size = tox_events_get_group_topic_lock_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (!tox_event_group_topic_lock_pack(tox_events_get_group_topic_lock(events, i), bp)) {
            return false;
        }
    }
    return true;
}

bool tox_events_unpack_group_topic_lock(Tox_Events *events, Bin_Unpack *bu)
{
    Tox_Event_Group_Topic_Lock *event = tox_events_add_group_topic_lock(events);

    if (event == nullptr) {
        return false;
    }

    return tox_event_group_topic_lock_unpack(event, bu);
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_group_topic_lock(Tox *tox, uint32_t group_number, Tox_Group_Topic_Lock topic_lock,
        void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return;
    }

    Tox_Event_Group_Topic_Lock *group_topic_lock = tox_events_add_group_topic_lock(state->events);

    if (group_topic_lock == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return;
    }

    tox_event_group_topic_lock_set_group_number(group_topic_lock, group_number);
    tox_event_group_topic_lock_set_topic_lock(group_topic_lock, topic_lock);
}
