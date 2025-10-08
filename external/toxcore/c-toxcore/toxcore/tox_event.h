/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2022-2025 The TokTok team.
 */

#ifndef C_TOXCORE_TOXCORE_TOX_EVENT_H
#define C_TOXCORE_TOXCORE_TOX_EVENT_H

#include "attributes.h"
#include "bin_pack.h"
#include "bin_unpack.h"
#include "mem.h"
#include "tox_events.h"
#include "tox_private.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union Tox_Event_Data {
    /**
     * Opaque pointer just to check whether any value is set.
     */
    void *_Nullable value;

    Tox_Event_Conference_Connected *_Nullable conference_connected;
    Tox_Event_Conference_Invite *_Nullable conference_invite;
    Tox_Event_Conference_Message *_Nullable conference_message;
    Tox_Event_Conference_Peer_List_Changed *_Nullable conference_peer_list_changed;
    Tox_Event_Conference_Peer_Name *_Nullable conference_peer_name;
    Tox_Event_Conference_Title *_Nullable conference_title;
    Tox_Event_File_Chunk_Request *_Nullable file_chunk_request;
    Tox_Event_File_Recv *_Nullable file_recv;
    Tox_Event_File_Recv_Chunk *_Nullable file_recv_chunk;
    Tox_Event_File_Recv_Control *_Nullable file_recv_control;
    Tox_Event_Friend_Connection_Status *_Nullable friend_connection_status;
    Tox_Event_Friend_Lossless_Packet *_Nullable friend_lossless_packet;
    Tox_Event_Friend_Lossy_Packet *_Nullable friend_lossy_packet;
    Tox_Event_Friend_Message *_Nullable friend_message;
    Tox_Event_Friend_Name *_Nullable friend_name;
    Tox_Event_Friend_Read_Receipt *_Nullable friend_read_receipt;
    Tox_Event_Friend_Request *_Nullable friend_request;
    Tox_Event_Friend_Status *_Nullable friend_status;
    Tox_Event_Friend_Status_Message *_Nullable friend_status_message;
    Tox_Event_Friend_Typing *_Nullable friend_typing;
    Tox_Event_Self_Connection_Status *_Nullable self_connection_status;
    Tox_Event_Group_Peer_Name *_Nullable group_peer_name;
    Tox_Event_Group_Peer_Status *_Nullable group_peer_status;
    Tox_Event_Group_Topic *_Nullable group_topic;
    Tox_Event_Group_Privacy_State *_Nullable group_privacy_state;
    Tox_Event_Group_Voice_State *_Nullable group_voice_state;
    Tox_Event_Group_Topic_Lock *_Nullable group_topic_lock;
    Tox_Event_Group_Peer_Limit *_Nullable group_peer_limit;
    Tox_Event_Group_Password *_Nullable group_password;
    Tox_Event_Group_Message *_Nullable group_message;
    Tox_Event_Group_Private_Message *_Nullable group_private_message;
    Tox_Event_Group_Custom_Packet *_Nullable group_custom_packet;
    Tox_Event_Group_Custom_Private_Packet *_Nullable group_custom_private_packet;
    Tox_Event_Group_Invite *_Nullable group_invite;
    Tox_Event_Group_Peer_Join *_Nullable group_peer_join;
    Tox_Event_Group_Peer_Exit *_Nullable group_peer_exit;
    Tox_Event_Group_Self_Join *_Nullable group_self_join;
    Tox_Event_Group_Join_Fail *_Nullable group_join_fail;
    Tox_Event_Group_Moderation *_Nullable group_moderation;
    Tox_Event_Dht_Nodes_Response *_Nullable dht_nodes_response;
} Tox_Event_Data;

struct Tox_Event {
    Tox_Event_Type type;
    Tox_Event_Data data;
};

/**
 * Constructor.
 */
bool tox_event_construct(Tox_Event *_Nonnull event, Tox_Event_Type type, const Memory *_Nonnull mem);

Tox_Event_Conference_Connected *_Nullable tox_event_conference_connected_new(const Memory *_Nonnull mem);
Tox_Event_Conference_Invite *_Nullable tox_event_conference_invite_new(const Memory *_Nonnull mem);
Tox_Event_Conference_Message *_Nullable tox_event_conference_message_new(const Memory *_Nonnull mem);
Tox_Event_Conference_Peer_List_Changed *_Nullable tox_event_conference_peer_list_changed_new(const Memory *_Nonnull mem);
Tox_Event_Conference_Peer_Name *_Nullable tox_event_conference_peer_name_new(const Memory *_Nonnull mem);
Tox_Event_Conference_Title *_Nullable tox_event_conference_title_new(const Memory *_Nonnull mem);
Tox_Event_File_Chunk_Request *_Nullable tox_event_file_chunk_request_new(const Memory *_Nonnull mem);
Tox_Event_File_Recv_Chunk *_Nullable tox_event_file_recv_chunk_new(const Memory *_Nonnull mem);
Tox_Event_File_Recv_Control *_Nullable tox_event_file_recv_control_new(const Memory *_Nonnull mem);
Tox_Event_File_Recv *_Nullable tox_event_file_recv_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Connection_Status *_Nullable tox_event_friend_connection_status_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Lossless_Packet *_Nullable tox_event_friend_lossless_packet_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Lossy_Packet *_Nullable tox_event_friend_lossy_packet_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Message *_Nullable tox_event_friend_message_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Name *_Nullable tox_event_friend_name_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Read_Receipt *_Nullable tox_event_friend_read_receipt_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Request *_Nullable tox_event_friend_request_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Status_Message *_Nullable tox_event_friend_status_message_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Status *_Nullable tox_event_friend_status_new(const Memory *_Nonnull mem);
Tox_Event_Friend_Typing *_Nullable tox_event_friend_typing_new(const Memory *_Nonnull mem);
Tox_Event_Self_Connection_Status *_Nullable tox_event_self_connection_status_new(const Memory *_Nonnull mem);
Tox_Event_Group_Peer_Name *_Nullable tox_event_group_peer_name_new(const Memory *_Nonnull mem);
Tox_Event_Group_Peer_Status *_Nullable tox_event_group_peer_status_new(const Memory *_Nonnull mem);
Tox_Event_Group_Topic *_Nullable tox_event_group_topic_new(const Memory *_Nonnull mem);
Tox_Event_Group_Privacy_State *_Nullable tox_event_group_privacy_state_new(const Memory *_Nonnull mem);
Tox_Event_Group_Voice_State *_Nullable tox_event_group_voice_state_new(const Memory *_Nonnull mem);
Tox_Event_Group_Topic_Lock *_Nullable tox_event_group_topic_lock_new(const Memory *_Nonnull mem);
Tox_Event_Group_Peer_Limit *_Nullable tox_event_group_peer_limit_new(const Memory *_Nonnull mem);
Tox_Event_Group_Password *_Nullable tox_event_group_password_new(const Memory *_Nonnull mem);
Tox_Event_Group_Message *_Nullable tox_event_group_message_new(const Memory *_Nonnull mem);
Tox_Event_Group_Private_Message *_Nullable tox_event_group_private_message_new(const Memory *_Nonnull mem);
Tox_Event_Group_Custom_Packet *_Nullable tox_event_group_custom_packet_new(const Memory *_Nonnull mem);
Tox_Event_Group_Custom_Private_Packet *_Nullable tox_event_group_custom_private_packet_new(const Memory *_Nonnull mem);
Tox_Event_Group_Invite *_Nullable tox_event_group_invite_new(const Memory *_Nonnull mem);
Tox_Event_Group_Peer_Join *_Nullable tox_event_group_peer_join_new(const Memory *_Nonnull mem);
Tox_Event_Group_Peer_Exit *_Nullable tox_event_group_peer_exit_new(const Memory *_Nonnull mem);
Tox_Event_Group_Self_Join *_Nullable tox_event_group_self_join_new(const Memory *_Nonnull mem);
Tox_Event_Group_Join_Fail *_Nullable tox_event_group_join_fail_new(const Memory *_Nonnull mem);
Tox_Event_Group_Moderation *_Nullable tox_event_group_moderation_new(const Memory *_Nonnull mem);
Tox_Event_Dht_Nodes_Response *_Nullable tox_event_dht_nodes_response_new(const Memory *_Nonnull mem);

/**
 * Destructor.
 */
void tox_event_destruct(Tox_Event *_Nullable event, const Memory *_Nonnull mem);

void tox_event_conference_connected_free(Tox_Event_Conference_Connected *_Nullable conference_connected, const Memory *_Nonnull mem);
void tox_event_conference_invite_free(Tox_Event_Conference_Invite *_Nullable conference_invite, const Memory *_Nonnull mem);
void tox_event_conference_message_free(Tox_Event_Conference_Message *_Nullable conference_message, const Memory *_Nonnull mem);
void tox_event_conference_peer_list_changed_free(Tox_Event_Conference_Peer_List_Changed *_Nullable conference_peer_list_changed, const Memory *_Nonnull mem);
void tox_event_conference_peer_name_free(Tox_Event_Conference_Peer_Name *_Nullable conference_peer_name, const Memory *_Nonnull mem);
void tox_event_conference_title_free(Tox_Event_Conference_Title *_Nullable conference_title, const Memory *_Nonnull mem);
void tox_event_file_chunk_request_free(Tox_Event_File_Chunk_Request *_Nullable file_chunk_request, const Memory *_Nonnull mem);
void tox_event_file_recv_chunk_free(Tox_Event_File_Recv_Chunk *_Nullable file_recv_chunk, const Memory *_Nonnull mem);
void tox_event_file_recv_control_free(Tox_Event_File_Recv_Control *_Nullable file_recv_control, const Memory *_Nonnull mem);
void tox_event_file_recv_free(Tox_Event_File_Recv *_Nullable file_recv, const Memory *_Nonnull mem);
void tox_event_friend_connection_status_free(Tox_Event_Friend_Connection_Status *_Nullable friend_connection_status, const Memory *_Nonnull mem);
void tox_event_friend_lossless_packet_free(Tox_Event_Friend_Lossless_Packet *_Nullable friend_lossless_packet, const Memory *_Nonnull mem);
void tox_event_friend_lossy_packet_free(Tox_Event_Friend_Lossy_Packet *_Nullable friend_lossy_packet, const Memory *_Nonnull mem);
void tox_event_friend_message_free(Tox_Event_Friend_Message *_Nullable friend_message, const Memory *_Nonnull mem);
void tox_event_friend_name_free(Tox_Event_Friend_Name *_Nullable friend_name, const Memory *_Nonnull mem);
void tox_event_friend_read_receipt_free(Tox_Event_Friend_Read_Receipt *_Nullable friend_read_receipt, const Memory *_Nonnull mem);
void tox_event_friend_request_free(Tox_Event_Friend_Request *_Nullable friend_request, const Memory *_Nonnull mem);
void tox_event_friend_status_message_free(Tox_Event_Friend_Status_Message *_Nullable friend_status_message, const Memory *_Nonnull mem);
void tox_event_friend_status_free(Tox_Event_Friend_Status *_Nullable friend_status, const Memory *_Nonnull mem);
void tox_event_friend_typing_free(Tox_Event_Friend_Typing *_Nullable friend_typing, const Memory *_Nonnull mem);
void tox_event_self_connection_status_free(Tox_Event_Self_Connection_Status *_Nullable self_connection_status, const Memory *_Nonnull mem);
void tox_event_group_peer_name_free(Tox_Event_Group_Peer_Name *_Nullable group_peer_name, const Memory *_Nonnull mem);
void tox_event_group_peer_status_free(Tox_Event_Group_Peer_Status *_Nullable group_peer_status, const Memory *_Nonnull mem);
void tox_event_group_topic_free(Tox_Event_Group_Topic *_Nullable group_topic, const Memory *_Nonnull mem);
void tox_event_group_privacy_state_free(Tox_Event_Group_Privacy_State *_Nullable group_privacy_state, const Memory *_Nonnull mem);
void tox_event_group_voice_state_free(Tox_Event_Group_Voice_State *_Nullable group_voice_state, const Memory *_Nonnull mem);
void tox_event_group_topic_lock_free(Tox_Event_Group_Topic_Lock *_Nullable group_topic_lock, const Memory *_Nonnull mem);
void tox_event_group_peer_limit_free(Tox_Event_Group_Peer_Limit *_Nullable group_peer_limit, const Memory *_Nonnull mem);
void tox_event_group_password_free(Tox_Event_Group_Password *_Nullable group_password, const Memory *_Nonnull mem);
void tox_event_group_message_free(Tox_Event_Group_Message *_Nullable group_message, const Memory *_Nonnull mem);
void tox_event_group_private_message_free(Tox_Event_Group_Private_Message *_Nullable group_private_message, const Memory *_Nonnull mem);
void tox_event_group_custom_packet_free(Tox_Event_Group_Custom_Packet *_Nullable group_custom_packet, const Memory *_Nonnull mem);
void tox_event_group_custom_private_packet_free(Tox_Event_Group_Custom_Private_Packet *_Nullable group_custom_private_packet, const Memory *_Nonnull mem);
void tox_event_group_invite_free(Tox_Event_Group_Invite *_Nullable group_invite, const Memory *_Nonnull mem);
void tox_event_group_peer_join_free(Tox_Event_Group_Peer_Join *_Nullable group_peer_join, const Memory *_Nonnull mem);
void tox_event_group_peer_exit_free(Tox_Event_Group_Peer_Exit *_Nullable group_peer_exit, const Memory *_Nonnull mem);
void tox_event_group_self_join_free(Tox_Event_Group_Self_Join *_Nullable group_self_join, const Memory *_Nonnull mem);
void tox_event_group_join_fail_free(Tox_Event_Group_Join_Fail *_Nullable group_join_fail, const Memory *_Nonnull mem);
void tox_event_group_moderation_free(Tox_Event_Group_Moderation *_Nullable group_moderation, const Memory *_Nonnull mem);
void tox_event_dht_nodes_response_free(Tox_Event_Dht_Nodes_Response *_Nullable dht_nodes_response, const Memory *_Nonnull mem);

/**
 * Pack into msgpack.
 */
bool tox_event_pack(const Tox_Event *_Nonnull event, Bin_Pack *_Nonnull bp);

bool tox_event_conference_connected_pack(const Tox_Event_Conference_Connected *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_conference_invite_pack(const Tox_Event_Conference_Invite *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_conference_message_pack(const Tox_Event_Conference_Message *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_conference_peer_list_changed_pack(const Tox_Event_Conference_Peer_List_Changed *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_conference_peer_name_pack(const Tox_Event_Conference_Peer_Name *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_conference_title_pack(const Tox_Event_Conference_Title *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_file_chunk_request_pack(const Tox_Event_File_Chunk_Request *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_file_recv_chunk_pack(const Tox_Event_File_Recv_Chunk *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_file_recv_control_pack(const Tox_Event_File_Recv_Control *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_file_recv_pack(const Tox_Event_File_Recv *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_connection_status_pack(const Tox_Event_Friend_Connection_Status *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_lossless_packet_pack(const Tox_Event_Friend_Lossless_Packet *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_lossy_packet_pack(const Tox_Event_Friend_Lossy_Packet *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_message_pack(const Tox_Event_Friend_Message *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_name_pack(const Tox_Event_Friend_Name *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_read_receipt_pack(const Tox_Event_Friend_Read_Receipt *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_request_pack(const Tox_Event_Friend_Request *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_status_message_pack(const Tox_Event_Friend_Status_Message *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_status_pack(const Tox_Event_Friend_Status *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_friend_typing_pack(const Tox_Event_Friend_Typing *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_self_connection_status_pack(const Tox_Event_Self_Connection_Status *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_peer_name_pack(const Tox_Event_Group_Peer_Name *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_peer_status_pack(const Tox_Event_Group_Peer_Status *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_topic_pack(const Tox_Event_Group_Topic *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_privacy_state_pack(const Tox_Event_Group_Privacy_State *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_voice_state_pack(const Tox_Event_Group_Voice_State *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_topic_lock_pack(const Tox_Event_Group_Topic_Lock *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_peer_limit_pack(const Tox_Event_Group_Peer_Limit *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_password_pack(const Tox_Event_Group_Password *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_message_pack(const Tox_Event_Group_Message *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_private_message_pack(const Tox_Event_Group_Private_Message *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_custom_packet_pack(const Tox_Event_Group_Custom_Packet *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_custom_private_packet_pack(const Tox_Event_Group_Custom_Private_Packet *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_invite_pack(const Tox_Event_Group_Invite *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_peer_join_pack(const Tox_Event_Group_Peer_Join *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_peer_exit_pack(const Tox_Event_Group_Peer_Exit *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_self_join_pack(const Tox_Event_Group_Self_Join *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_join_fail_pack(const Tox_Event_Group_Join_Fail *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_group_moderation_pack(const Tox_Event_Group_Moderation *_Nonnull event, Bin_Pack *_Nonnull bp);
bool tox_event_dht_nodes_response_pack(const Tox_Event_Dht_Nodes_Response *_Nonnull event, Bin_Pack *_Nonnull bp);

/**
 * Unpack from msgpack.
 */
bool tox_event_unpack_into(Tox_Event *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);

bool tox_event_conference_connected_unpack(Tox_Event_Conference_Connected *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_conference_invite_unpack(Tox_Event_Conference_Invite *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_conference_message_unpack(Tox_Event_Conference_Message *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_conference_peer_list_changed_unpack(Tox_Event_Conference_Peer_List_Changed *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_conference_peer_name_unpack(Tox_Event_Conference_Peer_Name *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_conference_title_unpack(Tox_Event_Conference_Title *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_file_chunk_request_unpack(Tox_Event_File_Chunk_Request *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_file_recv_chunk_unpack(Tox_Event_File_Recv_Chunk *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_file_recv_control_unpack(Tox_Event_File_Recv_Control *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_file_recv_unpack(Tox_Event_File_Recv *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_connection_status_unpack(Tox_Event_Friend_Connection_Status *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_lossless_packet_unpack(Tox_Event_Friend_Lossless_Packet *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_lossy_packet_unpack(Tox_Event_Friend_Lossy_Packet *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_message_unpack(Tox_Event_Friend_Message *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_name_unpack(Tox_Event_Friend_Name *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_read_receipt_unpack(Tox_Event_Friend_Read_Receipt *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_request_unpack(Tox_Event_Friend_Request *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_status_message_unpack(Tox_Event_Friend_Status_Message *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_status_unpack(Tox_Event_Friend_Status *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_friend_typing_unpack(Tox_Event_Friend_Typing *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_self_connection_status_unpack(Tox_Event_Self_Connection_Status *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_peer_name_unpack(Tox_Event_Group_Peer_Name *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_peer_status_unpack(Tox_Event_Group_Peer_Status *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_topic_unpack(Tox_Event_Group_Topic *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_privacy_state_unpack(Tox_Event_Group_Privacy_State *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_voice_state_unpack(Tox_Event_Group_Voice_State *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_topic_lock_unpack(Tox_Event_Group_Topic_Lock *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_peer_limit_unpack(Tox_Event_Group_Peer_Limit *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_password_unpack(Tox_Event_Group_Password *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_message_unpack(Tox_Event_Group_Message *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_private_message_unpack(Tox_Event_Group_Private_Message *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_custom_packet_unpack(Tox_Event_Group_Custom_Packet *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_custom_private_packet_unpack(Tox_Event_Group_Custom_Private_Packet *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_invite_unpack(Tox_Event_Group_Invite *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_peer_join_unpack(Tox_Event_Group_Peer_Join *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_peer_exit_unpack(Tox_Event_Group_Peer_Exit *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_self_join_unpack(Tox_Event_Group_Self_Join *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_join_fail_unpack(Tox_Event_Group_Join_Fail *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_group_moderation_unpack(Tox_Event_Group_Moderation *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);
bool tox_event_dht_nodes_response_unpack(Tox_Event_Dht_Nodes_Response *_Nonnull *_Nonnull event, Bin_Unpack *_Nonnull bu, const Memory *_Nonnull mem);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_TOX_EVENT_H */
