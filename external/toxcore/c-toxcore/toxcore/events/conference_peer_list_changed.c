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


struct Tox_Event_Conference_Peer_List_Changed {
    uint32_t conference_number;
};

non_null()
static void tox_event_conference_peer_list_changed_set_conference_number(Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed,
        uint32_t conference_number)
{
    assert(conference_peer_list_changed != nullptr);
    conference_peer_list_changed->conference_number = conference_number;
}
uint32_t tox_event_conference_peer_list_changed_get_conference_number(const Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed)
{
    assert(conference_peer_list_changed != nullptr);
    return conference_peer_list_changed->conference_number;
}

non_null()
static void tox_event_conference_peer_list_changed_construct(Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed)
{
    *conference_peer_list_changed = (Tox_Event_Conference_Peer_List_Changed) {
        0
    };
}
non_null()
static void tox_event_conference_peer_list_changed_destruct(Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed, const Memory *mem)
{
    return;
}

bool tox_event_conference_peer_list_changed_pack(
    const Tox_Event_Conference_Peer_List_Changed *event, Bin_Pack *bp)
{
    assert(event != nullptr);
    return bin_pack_array(bp, 2)
           && bin_pack_u32(bp, TOX_EVENT_CONFERENCE_PEER_LIST_CHANGED)
           && bin_pack_u32(bp, event->conference_number);
}

non_null()
static bool tox_event_conference_peer_list_changed_unpack_into(
    Tox_Event_Conference_Peer_List_Changed *event, Bin_Unpack *bu)
{
    assert(event != nullptr);
    return bin_unpack_u32(bu, &event->conference_number);
}


/*****************************************************
 *
 * :: new/free/add/get/size/unpack
 *
 *****************************************************/

const Tox_Event_Conference_Peer_List_Changed *tox_event_get_conference_peer_list_changed(const Tox_Event *event)
{
    return event->type == TOX_EVENT_CONFERENCE_PEER_LIST_CHANGED ? event->data.conference_peer_list_changed : nullptr;
}

Tox_Event_Conference_Peer_List_Changed *tox_event_conference_peer_list_changed_new(const Memory *mem)
{
    Tox_Event_Conference_Peer_List_Changed *const conference_peer_list_changed =
        (Tox_Event_Conference_Peer_List_Changed *)mem_alloc(mem, sizeof(Tox_Event_Conference_Peer_List_Changed));

    if (conference_peer_list_changed == nullptr) {
        return nullptr;
    }

    tox_event_conference_peer_list_changed_construct(conference_peer_list_changed);
    return conference_peer_list_changed;
}

void tox_event_conference_peer_list_changed_free(Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed, const Memory *mem)
{
    if (conference_peer_list_changed != nullptr) {
        tox_event_conference_peer_list_changed_destruct(conference_peer_list_changed, mem);
    }
    mem_delete(mem, conference_peer_list_changed);
}

non_null()
static Tox_Event_Conference_Peer_List_Changed *tox_events_add_conference_peer_list_changed(Tox_Events *events, const Memory *mem)
{
    Tox_Event_Conference_Peer_List_Changed *const conference_peer_list_changed = tox_event_conference_peer_list_changed_new(mem);

    if (conference_peer_list_changed == nullptr) {
        return nullptr;
    }

    Tox_Event event;
    event.type = TOX_EVENT_CONFERENCE_PEER_LIST_CHANGED;
    event.data.conference_peer_list_changed = conference_peer_list_changed;

    tox_events_add(events, &event);
    return conference_peer_list_changed;
}

const Tox_Event_Conference_Peer_List_Changed *tox_events_get_conference_peer_list_changed(const Tox_Events *events, uint32_t index)
{
    uint32_t conference_peer_list_changed_index = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (conference_peer_list_changed_index > index) {
            return nullptr;
        }

        if (events->events[i].type == TOX_EVENT_CONFERENCE_PEER_LIST_CHANGED) {
            const Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed = events->events[i].data.conference_peer_list_changed;
            if (conference_peer_list_changed_index == index) {
                return conference_peer_list_changed;
            }
            ++conference_peer_list_changed_index;
        }
    }

    return nullptr;
}

uint32_t tox_events_get_conference_peer_list_changed_size(const Tox_Events *events)
{
    uint32_t conference_peer_list_changed_size = 0;
    const uint32_t size = tox_events_get_size(events);

    for (uint32_t i = 0; i < size; ++i) {
        if (events->events[i].type == TOX_EVENT_CONFERENCE_PEER_LIST_CHANGED) {
            ++conference_peer_list_changed_size;
        }
    }

    return conference_peer_list_changed_size;
}

bool tox_event_conference_peer_list_changed_unpack(
    Tox_Event_Conference_Peer_List_Changed **event, Bin_Unpack *bu, const Memory *mem)
{
    assert(event != nullptr);
    *event = tox_event_conference_peer_list_changed_new(mem);

    if (*event == nullptr) {
        return false;
    }

    return tox_event_conference_peer_list_changed_unpack_into(*event, bu);
}

non_null()
static Tox_Event_Conference_Peer_List_Changed *tox_event_conference_peer_list_changed_alloc(void *user_data)
{
    Tox_Events_State *state = tox_events_alloc(user_data);
    assert(state != nullptr);

    if (state->events == nullptr) {
        return nullptr;
    }

    Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed = tox_events_add_conference_peer_list_changed(state->events, state->mem);

    if (conference_peer_list_changed == nullptr) {
        state->error = TOX_ERR_EVENTS_ITERATE_MALLOC;
        return nullptr;
    }

    return conference_peer_list_changed;
}


/*****************************************************
 *
 * :: event handler
 *
 *****************************************************/


void tox_events_handle_conference_peer_list_changed(Tox *tox, uint32_t conference_number,
        void *user_data)
{
    Tox_Event_Conference_Peer_List_Changed *conference_peer_list_changed = tox_event_conference_peer_list_changed_alloc(user_data);

    if (conference_peer_list_changed == nullptr) {
        return;
    }

    tox_event_conference_peer_list_changed_set_conference_number(conference_peer_list_changed, conference_number);
}
