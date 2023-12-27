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
#include "../tox_private.h"


/*****************************************************
 *
 * :: struct and accessors
 *
 *****************************************************/


struct Tox_Event_Friend_Request {
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
    uint8_t *message;
    uint32_t message_length;
};

non_null()
static bool tox_event_friend_request_set_public_key(Tox_Event_Friend_Request *friend_request, const uint8_t *public_key)
{
    assert(friend_request != nullptr);

    memcpy(friend_request->public_key, public_key, TOX_PUBLIC_KEY_SIZE);
    return true;
}
const uint8_t *tox_event_friend_request_get_public_key(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->public_key;
}

non_null()
static bool tox_event_friend_request_set_message(Tox_Event_Friend_Request *friend_request,
        const uint8_t *message, uint32_t message_length, const Memory *mem)
{
    assert(friend_request != nullptr);

    if (friend_request->message != nullptr) {
        mem_delete(mem, friend_request->message);
        friend_request->message = nullptr;
        friend_request->message_length = 0;
    }

    friend_request->message = (uint8_t *)mem_balloc(mem, message_length * sizeof(uint8_t));

    if (friend_request->message == nullptr) {
        return false;
    }

    memcpy(friend_request->message, message, message_length * sizeof(uint8_t));
    friend_request->message_length = message_length;
    return true;
}
uint32_t tox_event_friend_request_get_message_length(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->message_length;
}
const uint8_t *tox_event_friend_request_get_message(const Tox_Event_Friend_Request *friend_request)
{
    assert(friend_request != nullptr);
    return friend_request->message;
}

non_null()
static void tox_event_friend_request_construct(Tox_Event_Friend_Request *friend_request)
{
    *friend_request = (Tox_Event_Friend_Request) {
        0
    };
}
non_null()
static void tox_event_friend_request_destruct(Tox_Event_Friend_Request *friend_request, const Memory *mem)
{
    mem_delete(mem, friend_request->message);
}

bool tox_event_friend_request_pack(
    const Tox_Event_Friend_Request *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_FRIEND_REQUEST)
           && bin_pack_array(bp, 2)
           && bin_pack_bin(bp, event->public_key, TOX_PUBLIC_KEY_SIZE)
           && bin_pack_bin(bp, event->message, event->message_length);
}

non_null()
static bool tox_event_friend_request_unpack_into(
    Tox_Event_Friend_Request *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 2, nullptr)) {
        return false;
    }

    return bin_unpack_bin_fixed(bu, event->public_key, TOX_PUBLIC_KEY_SIZE)
           && bin_unpack_bin(bu, &event->message, &event->message_length);
}

const Tox_Event_Friend_Request *tox_event_get_friend_request(
    const Tox_Event *event)
{
    return event->type == TOX_EVENT_FRIEND_REQUEST ? event->data.friend_request : nullptr;
}

Tox_Event_Friend_Request *tox_event_friend_request_new(const Memory *mem)
{
    Tox_Event_Friend_Request *const friend_request =
        (Tox_Event_Friend_Request *)mem_alloc(mem, sizeof(Tox_Event_Friend_Request));

    if (friend_request == nullptr) {
        return nullptr;
    }

    tox_event_friend_request_construct(friend_request);
    return friend_request;
}

void tox_event_friend_request_free(Tox_Event_Friend_Request *friend_request, const Memory *mem)
{
    if (friend_request != nullptr) {
        tox_event_friend_request_destruct(friend_request, mem);
    }
    mem_delete(mem, friend_request);
}

non_null()
static Tox_Event_Friend_Request *tox_events_add_friend_request(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Friend_Request *const friend_request = tox_event_friend_request_new(mem);

    if (friend_request == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_FRIEND_REQUEST;
    event.data.friend_request = friend_request;

    tox_events_add(events, &event);
    return friend_request;
}

const Tox_Event_Friend_Request *tox_events_get_friend_request(
    const Tox_Events *events, uint32_t index)
{
    uint32_t friend_request_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (friend_request_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_FRIEND_REQUEST) {
            const Tox_Event_Friend_Request *friend_request = events->events[i].data.friend_request;
            if (friend_request_index == index) {
                return friend_request;
            }
            ++friend_request_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_friend_request_size(
    const Tox_Events *events)
{
    uint32_t friend_request_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_FRIEND_REQUEST) {
            ++friend_request_size;
        }
    }

    return friend_request_size;
}

bool tox_event_friend_request_unpack(
    Tox_Event_Friend_Request **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_friend_request_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_friend_request_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Friend_Request *tox_event_friend_request_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Friend_Request *friend_request = tox_events_add_friend_request(state->events, state->mem);

    if (friend_request == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return friend_request;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_friend_request(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length,
                                      void *user_data)
{
    Tox_Event_Friend_Request *friend_request = tox_event_friend_request_alloc(user_data);

    if (friend_request == nullptr) {
        return;
    }

    const Tox_System *sys = tox_get_system(tox);

    tox_event_friend_request_set_public_key(friend_request, public_key);
    tox_event_friend_request_set_message(friend_request, message, length, sys->mem);
}
