/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2018 The TokTok team.
 * Copyright © 2014 Tox project.
 */

#include "TCP_common.h"

#include <string.h>

#include "attributes.h"
#include "ccompat.h"
#include "crypto_core.h"
#include "logger.h"
#include "mem.h"
#include "network.h"

void wipe_priority_list(const Memory *mem, TCP_Priority_List *p)
{
    while (p != nullptr) {
        TCP_Priority_List *pp = p;
        p = p->next;
        mem_delete(mem, pp->data);
        mem_delete(mem, pp);
    }
}

/**
 * @retval 0 if pending data was sent completely
 * @retval -1 if it wasn't
 */
int send_pending_data_nonpriority(const Logger *logger, TCP_Connection *con)
{
    if (con->last_packet_length == 0) {
        return 0;
    }

    const uint16_t left = con->last_packet_length - con->last_packet_sent;
    const int len = net_send(con->ns, logger, con->sock, con->last_packet + con->last_packet_sent, left, &con->ip_port);

    if (len <= 0) {
        return -1;
    }

    if (len == left) {
        con->last_packet_length = 0;
        con->last_packet_sent = 0;
        return 0;
    }

    con->last_packet_sent += len;
    return -1;
}

/**
 * @retval 0 if pending data was sent completely
 * @retval -1 if it wasn't
 */
int send_pending_data(const Logger *logger, TCP_Connection *con)
{
    /* finish sending current non-priority packet */
    if (send_pending_data_nonpriority(logger, con) == -1) {
        return -1;
    }

    TCP_Priority_List *p = con->priority_queue_start;

    while (p != nullptr) {
        const uint16_t left = p->size - p->sent;
        const int len = net_send(con->ns, logger, con->sock, p->data + p->sent, left, &con->ip_port);

        if (len != left) {
            if (len > 0) {
                p->sent += len;
            }

            break;
        }

        TCP_Priority_List *pp = p;
        p = p->next;
        mem_delete(con->mem, pp->data);
        mem_delete(con->mem, pp);
    }

    con->priority_queue_start = p;

    if (p == nullptr) {
        con->priority_queue_end = nullptr;
        return 0;
    }

    return -1;
}

/**
 * @retval false on failure (only if mem_alloc fails)
 * @retval true on success
 */
non_null()
static bool add_priority(TCP_Connection *con, const uint8_t *packet, uint16_t size, uint16_t sent)
{
    TCP_Priority_List *p = con->priority_queue_end;
    TCP_Priority_List *new_list = (TCP_Priority_List *)mem_alloc(con->mem, sizeof(TCP_Priority_List));

    if (new_list == nullptr) {
        return false;
    }

    uint8_t *data = (uint8_t *)mem_balloc(con->mem, size);

    if (data == nullptr) {
        mem_delete(con->mem, new_list);
        return false;
    }

    memcpy(data, packet, size);
    new_list->data = data;
    new_list->size = size;

    new_list->next = nullptr;
    new_list->sent = sent;

    if (p != nullptr) {
        p->next = new_list;
    } else {
        con->priority_queue_start = new_list;
    }

    con->priority_queue_end = new_list;
    return true;
}

/**
 * @retval 1 on success.
 * @retval 0 if could not send packet.
 * @retval -1 on failure (connection must be killed).
 */
int write_packet_tcp_secure_connection(const Logger *logger, TCP_Connection *con, const uint8_t *data, uint16_t length,
                                       bool priority)
{
    if (length + CRYPTO_MAC_SIZE > MAX_PACKET_SIZE) {
        return -1;
    }

    bool sendpriority = true;

    if (send_pending_data(logger, con) == -1) {
        if (priority) {
            sendpriority = false;
        } else {
            return 0;
        }
    }

    const uint16_t packet_size = sizeof(uint16_t) + length + CRYPTO_MAC_SIZE;
    VLA(uint8_t, packet, packet_size);

    uint16_t c_length = net_htons(length + CRYPTO_MAC_SIZE);
    memcpy(packet, &c_length, sizeof(uint16_t));
    int len = encrypt_data_symmetric(con->shared_key, con->sent_nonce, data, length, packet + sizeof(uint16_t));

    if ((unsigned int)len != (packet_size - sizeof(uint16_t))) {
        return -1;
    }

    if (priority) {
        len = sendpriority ? net_send(con->ns, logger, con->sock, packet, packet_size, &con->ip_port) : 0;

        if (len <= 0) {
            len = 0;
        }

        increment_nonce(con->sent_nonce);

        if ((unsigned int)len == packet_size) {
            return 1;
        }

        return add_priority(con, packet, packet_size, len) ? 1 : 0;
    }

    len = net_send(con->ns, logger, con->sock, packet, packet_size, &con->ip_port);

    if (len <= 0) {
        return 0;
    }

    increment_nonce(con->sent_nonce);

    if ((unsigned int)len == packet_size) {
        return 1;
    }

    memcpy(con->last_packet, packet, packet_size);
    con->last_packet_length = packet_size;
    con->last_packet_sent = len;
    return 1;
}

/** @brief Read length bytes from socket.
 *
 * return length on success
 * return -1 on failure/no data in buffer.
 */
int read_tcp_packet(
    const Logger *logger, const Memory *mem, const Network *ns, Socket sock, uint8_t *data, uint16_t length, const IP_Port *ip_port)
{
    const uint16_t count = net_socket_data_recv_buffer(ns, sock);

    if (count < length) {
        if (count != 0) {
            // Only log when there are some bytes available, as empty buffer
            // is a very common case and this spams our logs.
            LOGGER_TRACE(logger, "recv buffer has %d bytes, but requested %d bytes", count, length);
        }
        return -1;
    }

    const int len = net_recv(ns, logger, sock, data, length, ip_port);

    if (len != length) {
        LOGGER_ERROR(logger, "FAIL recv packet");
        return -1;
    }

    return len;
}

/** @brief Read the next two bytes in TCP stream then convert them to
 * length (host byte order).
 *
 * return length on success
 * return 0 if nothing has been read from socket.
 * return -1 on failure.
 */
non_null()
static uint16_t read_tcp_length(const Logger *logger, const Memory *mem, const Network *ns, Socket sock, const IP_Port *ip_port)
{
    const uint16_t count = net_socket_data_recv_buffer(ns, sock);

    if (count >= sizeof(uint16_t)) {
        uint8_t length_buf[sizeof(uint16_t)];
        const int len = net_recv(ns, logger, sock, length_buf, sizeof(length_buf), ip_port);

        if (len != sizeof(uint16_t)) {
            LOGGER_ERROR(logger, "FAIL recv packet");
            return 0;
        }

        uint16_t length;
        net_unpack_u16(length_buf, &length);

        if (length > MAX_PACKET_SIZE) {
            LOGGER_ERROR(logger, "TCP packet too large: %d > %d", length, MAX_PACKET_SIZE);
            return -1;
        }

        return length;
    }

    return 0;
}

/**
 * @return length of received packet on success.
 * @retval 0 if could not read any packet.
 * @retval -1 on failure (connection must be killed).
 */
int read_packet_tcp_secure_connection(
    const Logger *logger, const Memory *mem, const Network *ns,
    Socket sock, uint16_t *next_packet_length,
    const uint8_t *shared_key, uint8_t *recv_nonce, uint8_t *data,
    uint16_t max_len, const IP_Port *ip_port)
{
    if (*next_packet_length == 0) {
        const uint16_t len = read_tcp_length(logger, mem, ns, sock, ip_port);

        if (len == (uint16_t) -1) {
            return -1;
        }

        if (len == 0) {
            return 0;
        }

        *next_packet_length = len;
    }

    if (max_len + CRYPTO_MAC_SIZE < *next_packet_length) {
        LOGGER_DEBUG(logger, "packet too large");
        return -1;
    }

    VLA(uint8_t, data_encrypted, (int) *next_packet_length);
    const int len_packet = read_tcp_packet(logger, mem, ns, sock, data_encrypted, *next_packet_length, ip_port);

    if (len_packet == -1) {
        return 0;
    }

    if (len_packet != *next_packet_length) {
        LOGGER_WARNING(logger, "invalid packet length: %d, expected %d", len_packet, *next_packet_length);
        return 0;
    }

    *next_packet_length = 0;

    const int len = decrypt_data_symmetric(shared_key, recv_nonce, data_encrypted, len_packet, data);

    if (len + CRYPTO_MAC_SIZE != len_packet) {
        LOGGER_ERROR(logger, "decrypted length %d does not match expected length %d", len + CRYPTO_MAC_SIZE, len_packet);
        return -1;
    }

    increment_nonce(recv_nonce);

    return len;
}
