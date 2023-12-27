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


struct Tox_Event_Conference_Invite {
    uint32_t friend_number;
    Tox_Conference_Type type;
    uint8_t *cookie;
    uint32_t cookie_length;
};

non_null()
static void tox_event_conference_invite_set_friend_number(Tox_Event_Conference_Invite *conference_invite,
        uint32_t friend_number)
{
    assert(conference_invite != nullptr);
    conference_invite->friend_number = friend_number;
}
uint32_t tox_event_conference_invite_get_friend_number(const Tox_Event_Conference_Invite *conference_invite)
{
    assert(conference_invite != nullptr);
    return conference_invite->friend_number;
}

non_null()
static void tox_event_conference_invite_set_type(Tox_Event_Conference_Invite *conference_invite,
        Tox_Conference_Type type)
{
    assert(conference_invite != nullptr);
    conference_invite->type = type;
}
Tox_Conference_Type tox_event_conference_invite_get_type(const Tox_Event_Conference_Invite *conference_invite)
{
    assert(conference_invite != nullptr);
    return conference_invite->type;
}

non_null()
static bool tox_event_conference_invite_set_cookie(Tox_Event_Conference_Invite *conference_invite,
        const uint8_t *cookie, uint32_t cookie_length)
{
    assert(conference_invite != nullptr);

    if (conference_invite->cookie != nullptr) {
        free(conference_invite->cookie);
        conference_invite->cookie = nullptr;
        conference_invite->cookie_length = 0;
    }

    conference_invite->cookie = (uint8_t *)malloc(cookie_length);

    if (conference_invite->cookie == nullptr) {
        return false;
    }

    memcpy(conference_invite->cookie, cookie, cookie_length);
    conference_invite->cookie_length = cookie_length;
    return true;
}
uint32_t tox_event_conference_invite_get_cookie_length(const Tox_Event_Conference_Invite *conference_invite)
{
    assert(conference_invite != nullptr);
    return conference_invite->cookie_length;
}
const uint8_t *tox_event_conference_invite_get_cookie(const Tox_Event_Conference_Invite *conference_invite)
{
    assert(conference_invite != nullptr);
    return conference_invite->cookie;
}

non_null()
static void tox_event_conference_invite_construct(Tox_Event_Conference_Invite *conference_invite)
{
    *conference_invite = (Tox_Event_Conference_Invite) {
        0
    };
}
non_null()
static void tox_event_conference_invite_destruct(Tox_Event_Conference_Invite *conference_invite, const Memory *mem)
{
    free(conference_invite->cookie);
}

bool tox_event_conference_invite_pack(
    const Tox_Event_Conference_Invite *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_CONFERENCE_INVITE)
           && bin_pack_array(bp, 3)
           && bin_pack_u32(bp, event->friend_number)
           && bin_pack_u32(bp, event->type)
           && bin_pack_bin(bp, event->cookie, event->cookie_length);
}

non_null()
static bool tox_event_conference_invite_unpack_into(
    Tox_Event_Conference_Invite *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 3, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->friend_number)
           && tox_conference_type_unpack(bu, &event->type)
           && bin_unpack_bin(bu, &event->cookie, &event->cookie_length);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Conference_Invite *tox_event_get_conference_invite(const Tox_Event *event)
{
    return event->type == TOX_EVENT_CONFERENCE_INVITE ? event->data.conference_invite : nullptr;
}

Tox_Event_Conference_Invite *tox_event_conference_invite_new(const Memory *mem)
{
    Tox_Event_Conference_Invite *const conference_invite =
        (Tox_Event_Conference_Invite *)mem_alloc(mem, sizeof(Tox_Event_Conference_Invite));

    if (conference_invite == nullptr) {
        return nullptr;
    }

    tox_event_conference_invite_construct(conference_invite);
    return conference_invite;
}

void tox_event_conference_invite_free(Tox_Event_Conference_Invite *conference_invite, const Memory *mem)
{
    if (conference_invite != nullptr) {
        tox_event_conference_invite_destruct(conference_invite, mem);
    }
    mem_delete(mem, conference_invite);
}

non_null()
static Tox_Event_Conference_Invite *tox_events_add_conference_invite(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Conference_Invite *const conference_invite = tox_event_conference_invite_new(mem);

    if (conference_invite == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_CONFERENCE_INVITE;
    event.data.conference_invite = conference_invite;

    tox_events_add(events, &event);
    return conference_invite;
}

const Tox_Event_Conference_Invite *tox_events_get_conference_invite(const Tox_Events *events, uint32_t index)
{
    uint32_t conference_invite_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (conference_invite_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_CONFERENCE_INVITE) {
            const Tox_Event_Conference_Invite *conference_invite = events->events[i].data.conference_invite;
            if (conference_invite_index == index) {
                return conference_invite;
            }
            ++conference_invite_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_conference_invite_size(const Tox_Events *events)
{
    uint32_t conference_invite_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_CONFERENCE_INVITE) {
            ++conference_invite_size;
        }
    }

    return conference_invite_size;
}

bool tox_event_conference_invite_unpack(
    Tox_Event_Conference_Invite **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_conference_invite_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_conference_invite_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Conference_Invite *tox_event_conference_invite_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Conference_Invite *conference_invite = tox_events_add_conference_invite(state->events, state->mem);

    if (conference_invite == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return conference_invite;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_conference_invite(Tox *tox, uint32_t friend_number, Tox_Conference_Type type, const uint8_t *cookie, size_t length,
        void *user_data)
{
    Tox_Event_Conference_Invite *conference_invite = tox_event_conference_invite_alloc(user_data);

    if (conference_invite == nullptr) {
        return;
    }

    tox_event_conference_invite_set_friend_number(conference_invite, friend_number);
    tox_event_conference_invite_set_type(conference_invite, type);
    tox_event_conference_invite_set_cookie(conference_invite, cookie, length);
}
