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


struct Tox_Event_Conference_Message {
    uint32_t conference_number;
    uint32_t peer_number;
    Tox_Message_Type type;
    uint8_t *message;
    uint32_t message_length;
};

non_null()
static void tox_event_conference_message_set_conference_number(Tox_Event_Conference_Message *conference_message,
        uint32_t conference_number)
{
    assert(conference_message != nullptr);
    conference_message->conference_number = conference_number;
}
uint32_t tox_event_conference_message_get_conference_number(const Tox_Event_Conference_Message *conference_message)
{
    assert(conference_message != nullptr);
    return conference_message->conference_number;
}

non_null()
static void tox_event_conference_message_set_peer_number(Tox_Event_Conference_Message *conference_message,
        uint32_t peer_number)
{
    assert(conference_message != nullptr);
    conference_message->peer_number = peer_number;
}
uint32_t tox_event_conference_message_get_peer_number(const Tox_Event_Conference_Message *conference_message)
{
    assert(conference_message != nullptr);
    return conference_message->peer_number;
}

non_null()
static void tox_event_conference_message_set_type(Tox_Event_Conference_Message *conference_message,
        Tox_Message_Type type)
{
    assert(conference_message != nullptr);
    conference_message->type = type;
}
Tox_Message_Type tox_event_conference_message_get_type(const Tox_Event_Conference_Message *conference_message)
{
    assert(conference_message != nullptr);
    return conference_message->type;
}

non_null()
static bool tox_event_conference_message_set_message(Tox_Event_Conference_Message *conference_message,
        const uint8_t *message, uint32_t message_length)
{
    assert(conference_message != nullptr);

    if (conference_message->message != nullptr) {
        free(conference_message->message);
        conference_message->message = nullptr;
        conference_message->message_length = 0;
    }

    conference_message->message = (uint8_t *)malloc(message_length);

    if (conference_message->message == nullptr) {
        return false;
    }

    memcpy(conference_message->message, message, message_length);
    conference_message->message_length = message_length;
    return true;
}
uint32_t tox_event_conference_message_get_message_length(const Tox_Event_Conference_Message *conference_message)
{
    assert(conference_message != nullptr);
    return conference_message->message_length;
}
const uint8_t *tox_event_conference_message_get_message(const Tox_Event_Conference_Message *conference_message)
{
    assert(conference_message != nullptr);
    return conference_message->message;
}

non_null()
static void tox_event_conference_message_construct(Tox_Event_Conference_Message *conference_message)
{
    *conference_message = (Tox_Event_Conference_Message) {
        0
    };
}
non_null()
static void tox_event_conference_message_destruct(Tox_Event_Conference_Message *conference_message, const Memory *mem)
{
    free(conference_message->message);
}

bool tox_event_conference_message_pack(
    const Tox_Event_Conference_Message *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_CONFERENCE_MESSAGE)
           && bin_pack_array(bp, 4)
           && bin_pack_u32(bp, event->conference_number)
           && bin_pack_u32(bp, event->peer_number)
           && bin_pack_u32(bp, event->type)
           && bin_pack_bin(bp, event->message, event->message_length);
}

non_null()
static bool tox_event_conference_message_unpack_into(
    Tox_Event_Conference_Message *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    if (!bin_unpack_array_fixed(bu, 4, nullptr)) {
        return false;
    }

    return bin_unpack_u32(bu, &event->conference_number)
           && bin_unpack_u32(bu, &event->peer_number)
           && tox_message_type_unpack(bu, &event->type)
           && bin_unpack_bin(bu, &event->message, &event->message_length);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Conference_Message *tox_event_get_conference_message(const Tox_Event *event)
{
    return event->type == TOX_EVENT_CONFERENCE_MESSAGE ? event->data.conference_message : nullptr;
}

Tox_Event_Conference_Message *tox_event_conference_message_new(const Memory *mem)
{
    Tox_Event_Conference_Message *const conference_message =
        (Tox_Event_Conference_Message *)mem_alloc(mem, sizeof(Tox_Event_Conference_Message));

    if (conference_message == nullptr) {
        return nullptr;
    }

    tox_event_conference_message_construct(conference_message);
    return conference_message;
}

void tox_event_conference_message_free(Tox_Event_Conference_Message *conference_message, const Memory *mem)
{
    if (conference_message != nullptr) {
        tox_event_conference_message_destruct(conference_message, mem);
    }
    mem_delete(mem, conference_message);
}

non_null()
static Tox_Event_Conference_Message *tox_events_add_conference_message(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Conference_Message *const conference_message = tox_event_conference_message_new(mem);

    if (conference_message == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_CONFERENCE_MESSAGE;
    event.data.conference_message = conference_message;

    tox_events_add(events, &event);
    return conference_message;
}

const Tox_Event_Conference_Message *tox_events_get_conference_message(const Tox_Events *events, uint32_t index)
{
    uint32_t conference_message_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (conference_message_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_CONFERENCE_MESSAGE) {
            const Tox_Event_Conference_Message *conference_message = events->events[i].data.conference_message;
            if (conference_message_index == index) {
                return conference_message;
            }
            ++conference_message_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_conference_message_size(const Tox_Events *events)
{
    uint32_t conference_message_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_CONFERENCE_MESSAGE) {
            ++conference_message_size;
        }
    }

    return conference_message_size;
}

bool tox_event_conference_message_unpack(
    Tox_Event_Conference_Message **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_conference_message_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_conference_message_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Conference_Message *tox_event_conference_message_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Conference_Message *conference_message = tox_events_add_conference_message(state->events, state->mem);

    if (conference_message == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return conference_message;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_conference_message(Tox *tox, uint32_t conference_number, uint32_t peer_number, Tox_Message_Type type, const uint8_t *message, size_t length,
        void *user_data)
{
    Tox_Event_Conference_Message *conference_message = tox_event_conference_message_alloc(user_data);

    if (conference_message == nullptr) {
        return;
    }

    tox_event_conference_message_set_conference_number(conference_message, conference_number);
    tox_event_conference_message_set_peer_number(conference_message, peer_number);
    tox_event_conference_message_set_type(conference_message, type);
    tox_event_conference_message_set_message(conference_message, message, length);
}
