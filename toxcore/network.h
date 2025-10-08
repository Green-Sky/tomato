/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013 Tox project.
 */

/**
 * Datatypes, functions and includes for the core networking.
 */
#ifndef C_TOXCORE_TOXCORE_NETWORK_H
#define C_TOXCORE_TOXCORE_NETWORK_H

#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
#include <stdint.h>     // uint*_t

#include "attributes.h"
#include "bin_pack.h"
#include "logger.h"
#include "mem.h"
#include "net_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wrapper for sockaddr_storage and size.
 */
typedef struct Network_Addr Network_Addr;

typedef bitwise int Socket_Value;
typedef struct Socket {
    Socket_Value value;
} Socket;

int net_socket_to_native(Socket sock);
Socket net_socket_from_native(int sock);

typedef int net_close_cb(void *_Nullable obj, Socket sock);
typedef Socket net_accept_cb(void *_Nullable obj, Socket sock);
typedef int net_bind_cb(void *_Nullable obj, Socket sock, const Network_Addr *_Nonnull addr);
typedef int net_listen_cb(void *_Nullable obj, Socket sock, int backlog);
typedef int net_connect_cb(void *_Nullable obj, Socket sock, const Network_Addr *_Nonnull addr);
typedef int net_recvbuf_cb(void *_Nullable obj, Socket sock);
typedef int net_recv_cb(void *_Nullable obj, Socket sock, uint8_t *_Nonnull buf, size_t len);
typedef int net_recvfrom_cb(void *_Nullable obj, Socket sock, uint8_t *_Nonnull buf, size_t len, Network_Addr *_Nonnull addr);
typedef int net_send_cb(void *_Nullable obj, Socket sock, const uint8_t *_Nonnull buf, size_t len);
typedef int net_sendto_cb(void *_Nullable obj, Socket sock, const uint8_t *_Nonnull buf, size_t len, const Network_Addr *_Nonnull addr);
typedef Socket net_socket_cb(void *_Nullable obj, int domain, int type, int proto);
typedef int net_socket_nonblock_cb(void *_Nullable obj, Socket sock, bool nonblock);
typedef int net_getsockopt_cb(void *_Nullable obj, Socket sock, int level, int optname, void *_Nonnull optval, size_t *_Nonnull optlen);
typedef int net_setsockopt_cb(void *_Nullable obj, Socket sock, int level, int optname, const void *_Nonnull optval, size_t optlen);
typedef int net_getaddrinfo_cb(void *_Nullable obj, const Memory *_Nonnull mem, const char *_Nonnull address, int family, int protocol, Network_Addr *_Nullable *_Nonnull addrs);
typedef int net_freeaddrinfo_cb(void *_Nullable obj, const Memory *_Nonnull mem, Network_Addr *_Nullable addrs);

/** @brief Functions wrapping POSIX network functions.
 *
 * Refer to POSIX man pages for documentation of what these functions are
 * expected to do when providing alternative Network implementations.
 */
typedef struct Network_Funcs {
    net_close_cb *_Nullable close;
    net_accept_cb *_Nullable accept;
    net_bind_cb *_Nullable bind;
    net_listen_cb *_Nullable listen;
    net_connect_cb *_Nullable connect;
    net_recvbuf_cb *_Nullable recvbuf;
    net_recv_cb *_Nullable recv;
    net_recvfrom_cb *_Nullable recvfrom;
    net_send_cb *_Nullable send;
    net_sendto_cb *_Nullable sendto;
    net_socket_cb *_Nullable socket;
    net_socket_nonblock_cb *_Nullable socket_nonblock;
    net_getsockopt_cb *_Nullable getsockopt;
    net_setsockopt_cb *_Nullable setsockopt;
    net_getaddrinfo_cb *_Nullable getaddrinfo;
    net_freeaddrinfo_cb *_Nullable freeaddrinfo;
} Network_Funcs;

typedef struct Network {
    const Network_Funcs *_Nullable funcs;
    void *_Nullable obj;
} Network;

const Network *_Nullable os_network(void);

typedef struct Family {
    uint8_t value;
} Family;

bool net_family_is_unspec(Family family);
bool net_family_is_ipv4(Family family);
bool net_family_is_ipv6(Family family);
bool net_family_is_tcp_server(Family family);
bool net_family_is_tcp_client(Family family);
bool net_family_is_tcp_ipv4(Family family);
bool net_family_is_tcp_ipv6(Family family);
bool net_family_is_tox_tcp_ipv4(Family family);
bool net_family_is_tox_tcp_ipv6(Family family);

Family net_family_unspec(void);
Family net_family_ipv4(void);
Family net_family_ipv6(void);
Family net_family_tcp_server(void);
Family net_family_tcp_client(void);
Family net_family_tcp_ipv4(void);
Family net_family_tcp_ipv6(void);
Family net_family_tox_tcp_ipv4(void);
Family net_family_tox_tcp_ipv6(void);

#define MAX_UDP_PACKET_SIZE 2048

typedef enum Net_Packet_Type {
    NET_PACKET_PING_REQUEST         = 0x00, /* Ping request packet ID. */
    NET_PACKET_PING_RESPONSE        = 0x01, /* Ping response packet ID. */
    NET_PACKET_NODES_REQUEST        = 0x02, /* Nodes request packet ID. */
    NET_PACKET_NODES_RESPONSE       = 0x04, /* Nodes response packet ID. */
    NET_PACKET_COOKIE_REQUEST       = 0x18, /* Cookie request packet */
    NET_PACKET_COOKIE_RESPONSE      = 0x19, /* Cookie response packet */
    NET_PACKET_CRYPTO_HS            = 0x1a, /* Crypto handshake packet */
    NET_PACKET_CRYPTO_DATA          = 0x1b, /* Crypto data packet */
    NET_PACKET_CRYPTO               = 0x20, /* Encrypted data packet ID. */
    NET_PACKET_LAN_DISCOVERY        = 0x21, /* LAN discovery packet ID. */

    NET_PACKET_GC_HANDSHAKE         = 0x5a, /* Group chat handshake packet ID */
    NET_PACKET_GC_LOSSLESS          = 0x5b, /* Group chat lossless packet ID */
    NET_PACKET_GC_LOSSY             = 0x5c, /* Group chat lossy packet ID */

    /* See: `docs/Prevent_Tracking.txt` and `onion.{c,h}` */
    NET_PACKET_ONION_SEND_INITIAL   = 0x80,
    NET_PACKET_ONION_SEND_1         = 0x81,
    NET_PACKET_ONION_SEND_2         = 0x82,

    NET_PACKET_ANNOUNCE_REQUEST_OLD  = 0x83, /* TODO: DEPRECATE */
    NET_PACKET_ANNOUNCE_RESPONSE_OLD = 0x84, /* TODO: DEPRECATE */

    NET_PACKET_ONION_DATA_REQUEST   = 0x85,
    NET_PACKET_ONION_DATA_RESPONSE  = 0x86,
    NET_PACKET_ANNOUNCE_REQUEST     = 0x87,
    NET_PACKET_ANNOUNCE_RESPONSE    = 0x88,

    NET_PACKET_ONION_RECV_3         = 0x8c,
    NET_PACKET_ONION_RECV_2         = 0x8d,
    NET_PACKET_ONION_RECV_1         = 0x8e,

    NET_PACKET_FORWARD_REQUEST      = 0x90,
    NET_PACKET_FORWARDING           = 0x91,
    NET_PACKET_FORWARD_REPLY        = 0x92,

    NET_PACKET_DATA_SEARCH_REQUEST     = 0x93,
    NET_PACKET_DATA_SEARCH_RESPONSE    = 0x94,
    NET_PACKET_DATA_RETRIEVE_REQUEST   = 0x95,
    NET_PACKET_DATA_RETRIEVE_RESPONSE  = 0x96,
    NET_PACKET_STORE_ANNOUNCE_REQUEST  = 0x97,
    NET_PACKET_STORE_ANNOUNCE_RESPONSE = 0x98,

    BOOTSTRAP_INFO_PACKET_ID        = 0xf0, /* Only used for bootstrap nodes */

    NET_PACKET_MAX                  = 0xff, /* This type must remain within a single uint8. */
} Net_Packet_Type;

#define TOX_PORTRANGE_FROM 33445
#define TOX_PORTRANGE_TO   33545
#define TOX_PORT_DEFAULT   TOX_PORTRANGE_FROM

/** Redefinitions of variables for safe transfer over wire. */
#define TOX_AF_UNSPEC 0
#define TOX_AF_INET 2
#define TOX_AF_INET6 10
#define TOX_TCP_INET 130
#define TOX_TCP_INET6 138

#define TOX_SOCK_STREAM 1
#define TOX_SOCK_DGRAM 2

#define TOX_PROTO_TCP 1
#define TOX_PROTO_UDP 2

/** TCP related */
#define TCP_CLIENT_FAMILY (TOX_AF_INET6 + 1)
#define TCP_INET (TOX_AF_INET6 + 2)
#define TCP_INET6 (TOX_AF_INET6 + 3)
#define TCP_SERVER_FAMILY (TOX_AF_INET6 + 4)

#define SIZE_IP4 4
#define SIZE_IP6 16
#define SIZE_IP (1 + SIZE_IP6)
#define SIZE_PORT 2
#define SIZE_IPPORT (SIZE_IP + SIZE_PORT)

typedef union IP4 {
    uint32_t uint32;
    uint16_t uint16[2];
    uint8_t uint8[4];
} IP4;

IP4 get_ip4_loopback(void);
IP4 get_ip4_broadcast(void);

typedef union IP6 {
    uint8_t uint8[16];
    uint16_t uint16[8];
    uint32_t uint32[4];
    uint64_t uint64[2];
} IP6;

IP6 get_ip6_loopback(void);
IP6 get_ip6_broadcast(void);

typedef union IP_Union {
    IP4 v4;
    IP6 v6;
} IP_Union;

typedef struct IP {
    Family family;
    IP_Union ip;
} IP;

typedef struct IP_Port {
    IP ip;
    uint16_t port;
} IP_Port;

Socket net_socket(const Network *_Nonnull ns, Family domain, int type, int protocol);

/**
 * Check if socket is valid.
 *
 * @return true if valid, false otherwise.
 */
bool sock_valid(Socket sock);

Socket net_invalid_socket(void);

/**
 * Calls send(sockfd, buf, len, MSG_NOSIGNAL).
 *
 * @param ns System network object.
 * @param log Logger object.
 * @param sock Socket to send data with.
 * @param buf Data to send.
 * @param len Length of data.
 * @param ip_port IP and port to send data to.
 * @param net_profile Network profile to record the packet.
 */
int net_send(const Network *_Nonnull ns, const Logger *_Nonnull log, Socket sock, const uint8_t *_Nonnull buf, size_t len, const IP_Port *_Nonnull ip_port,
             Net_Profile *_Nullable net_profile);
/**
 * Calls recv(sockfd, buf, len, MSG_NOSIGNAL).
 *
 * @param ns System network object.
 * @param log Logger object.
 * @param sock Socket to receive data with.
 * @param buf Buffer to store received data.
 * @param len Length of buffer.
 * @param ip_port IP and port of the sender.
 */
int net_recv(const Network *_Nonnull ns, const Logger *_Nonnull log, Socket sock, uint8_t *_Nonnull buf, size_t len, const IP_Port *_Nonnull ip_port);
/**
 * Calls listen(sockfd, backlog).
 */
int net_listen(const Network *_Nonnull ns, Socket sock, int backlog);
/**
 * Calls accept(sockfd, nullptr, nullptr).
 */
Socket net_accept(const Network *_Nonnull ns, Socket sock);

/**
 * return the size of data in the tcp recv buffer.
 * return 0 on failure.
 */
uint16_t net_socket_data_recv_buffer(const Network *_Nonnull ns, Socket sock);

/** Convert values between host and network byte order. */
uint32_t net_htonl(uint32_t hostlong);
uint16_t net_htons(uint16_t hostshort);
uint32_t net_ntohl(uint32_t hostlong);
uint16_t net_ntohs(uint16_t hostshort);

size_t net_pack_bool(uint8_t *_Nonnull bytes, bool v);
size_t net_pack_u16(uint8_t *_Nonnull bytes, uint16_t v);
size_t net_pack_u32(uint8_t *_Nonnull bytes, uint32_t v);
size_t net_pack_u64(uint8_t *_Nonnull bytes, uint64_t v);

size_t net_unpack_bool(const uint8_t *_Nonnull bytes, bool *_Nonnull v);
size_t net_unpack_u16(const uint8_t *_Nonnull bytes, uint16_t *_Nonnull v);
size_t net_unpack_u32(const uint8_t *_Nonnull bytes, uint32_t *_Nonnull v);
size_t net_unpack_u64(const uint8_t *_Nonnull bytes, uint64_t *_Nonnull v);

/** Does the IP6 struct a contain an IPv4 address in an IPv6 one? */
bool ipv6_ipv4_in_v6(const IP6 *_Nonnull a);

#define TOX_ENABLE_IPV6_DEFAULT true

#define TOX_INET6_ADDRSTRLEN 66
#define TOX_INET_ADDRSTRLEN 22

/** this would be TOX_INET6_ADDRSTRLEN, but it might be too short for the error message */
#define IP_NTOA_LEN 96 // TODO(irungentoo): magic number. Why not INET6_ADDRSTRLEN ?

/** Contains a null terminated string of an IP address. */
typedef struct Ip_Ntoa {
    char     buf[IP_NTOA_LEN];  // a string formatted IP address or an error message.
    uint16_t length;  // the length of the string (not including the null byte).
    bool     ip_is_valid;  // if this is false `buf` will contain an error message.
} Ip_Ntoa;

/** @brief Converts IP into a null terminated string.
 *
 * Writes error message into the buffer on error.
 *
 * @param ip_str contains a buffer of the required size.
 *
 * @return Pointer to the buffer inside `ip_str` containing the IP string.
 */
const char *_Nonnull net_ip_ntoa(const IP *_Nonnull ip, Ip_Ntoa *_Nonnull ip_str);

/**
 * Parses IP structure into an address string.
 *
 * @param ip IP of TOX_AF_INET or TOX_AF_INET6 families.
 * @param length length of the address buffer.
 *   Must be at least TOX_INET_ADDRSTRLEN for TOX_AF_INET
 *   and TOX_INET6_ADDRSTRLEN for TOX_AF_INET6
 *
 * @param address dotted notation (IPv4: quad, IPv6: 16) or colon notation (IPv6).
 *
 * @return true on success, false on failure.
 */
bool ip_parse_addr(const IP *_Nonnull ip, char *_Nonnull address, size_t length);

/**
 * Directly parses the input into an IP structure.
 *
 * Tries IPv4 first, then IPv6.
 *
 * @param address dotted notation (IPv4: quad, IPv6: 16) or colon notation (IPv6).
 * @param to family and the value is set on success.
 *
 * @return true on success, false on failure.
 */
bool addr_parse_ip(const char *_Nonnull address, IP *_Nonnull to);

/**
 * Compares two IP structures.
 *
 * Unset means unequal.
 *
 * @return false when not equal or when uninitialized.
 */
bool ip_equal(const IP *_Nullable a, const IP *_Nullable b);
/**
 * Compares two IP_Port structures.
 *
 * Unset means unequal.
 *
 * @return false when not equal or when uninitialized.
 */
bool ipport_equal(const IP_Port *_Nullable a, const IP_Port *_Nullable b);
/**
 * @brief IP_Port comparison function with `memcmp` signature.
 *
 * Casts the void pointers to `IP_Port *` for comparison.
 *
 * @retval -1 if `a < b`
 * @retval 0 if `a == b`
 * @retval 1 if `a > b`
 */
int ipport_cmp_handler(const void *_Nonnull a, const void *_Nonnull b, size_t size);

/** nulls out ip */
void ip_reset(IP *_Nonnull ip);
/** nulls out ip_port */
void ipport_reset(IP_Port *_Nonnull ipport);
/** nulls out ip, sets family according to flag */
void ip_init(IP *_Nonnull ip, bool ipv6enabled);
/** checks if ip is valid */
bool ip_isset(const IP *_Nonnull ip);
/** checks if ip is valid */
bool ipport_isset(const IP_Port *_Nonnull ipport);
/** copies an ip structure (careful about direction) */
void ip_copy(IP *_Nonnull target, const IP *_Nonnull source);
/** copies an ip_port structure (careful about direction) */
void ipport_copy(IP_Port *_Nonnull target, const IP_Port *_Nonnull source);

/**
 * @brief Resolves string into an IP address.
 *
 * @param[in,out] ns Network object.
 * @param[in] address a hostname (or something parseable to an IP address).
 * @param[in,out] to to.family MUST be initialized, either set to a specific IP version
 *   (TOX_AF_INET/TOX_AF_INET6) or to the unspecified TOX_AF_UNSPEC (0), if both
 *   IP versions are acceptable.
 * @param[out] extra can be NULL and is only set in special circumstances, see returns.
 * @param[in] dns_enabled if false, DNS resolution is skipped.
 *
 * Returns in `*to` a matching address (IPv6 or IPv4).
 * Returns in `*extra`, if not NULL, an IPv4 address, if `to->family` was `TOX_AF_UNSPEC`.
 *
 * @return true on success, false on failure
 */
bool addr_resolve_or_parse_ip(const Network *_Nonnull ns, const Memory *_Nonnull mem, const char *_Nonnull address, IP *_Nonnull to, IP *_Nullable extra, bool dns_enabled);
/** @brief Function to receive data, ip and port of sender is put into ip_port.
 * Packet data is put into data.
 * Packet length is put into length.
 */
typedef int packet_handler_cb(void *_Nullable object, const IP_Port *_Nonnull source, const uint8_t *_Nonnull packet, uint16_t length, void *_Nullable userdata);

typedef struct Networking_Core Networking_Core;

Family net_family(const Networking_Core *_Nonnull net);
uint16_t net_port(const Networking_Core *_Nonnull net);

/** Close the socket. */
void kill_sock(const Network *_Nonnull ns, Socket sock);

/**
 * Set socket as nonblocking
 *
 * @return true on success, false on failure.
 */
bool set_socket_nonblock(const Network *_Nonnull ns, Socket sock);

/**
 * Set socket to not emit SIGPIPE
 *
 * @return true on success, false on failure.
 */
bool set_socket_nosigpipe(const Network *_Nonnull ns, Socket sock);

/**
 * Enable SO_REUSEADDR on socket.
 *
 * @return true on success, false on failure.
 */
bool set_socket_reuseaddr(const Network *_Nonnull ns, Socket sock);

/**
 * Set socket to dual (IPv4 + IPv6 socket)
 *
 * @return true on success, false on failure.
 */
bool set_socket_dualstack(const Network *_Nonnull ns, Socket sock);

/* Basic network functions: */

/**
 * An outgoing network packet.
 *
 * Use `send_packet` to send it to an IP/port endpoint.
 */
typedef struct Packet {
    const uint8_t *_Nonnull data;
    uint16_t length;
} Packet;

/**
 * Function to send a network packet to a given IP/port.
 */
int send_packet(const Networking_Core *_Nonnull net, const IP_Port *_Nonnull ip_port, Packet packet);

/**
 * Function to send packet(data) of length length to ip_port.
 *
 * @deprecated Use send_packet instead.
 */
int sendpacket(const Networking_Core *_Nonnull net, const IP_Port *_Nonnull ip_port, const uint8_t *_Nonnull data, uint16_t length);

/** Function to call when packet beginning with byte is received. */
void networking_registerhandler(Networking_Core *_Nonnull net, uint8_t byte, packet_handler_cb *_Nullable cb, void *_Nullable object);
/** Call this several times a second. */
void networking_poll(const Networking_Core *_Nonnull net, void *_Nullable userdata);
typedef enum Net_Err_Connect {
    NET_ERR_CONNECT_OK,
    NET_ERR_CONNECT_INVALID_FAMILY,
    NET_ERR_CONNECT_FAILED,
} Net_Err_Connect;

const char *_Nonnull net_err_connect_to_string(Net_Err_Connect err);

/** @brief Connect a socket to the address specified by the ip_port.
 *
 * @param[out] err Set to NET_ERR_CONNECT_OK on success, otherwise an error code.
 *
 * @retval true on success, false on failure.
 */
bool net_connect(const Network *_Nonnull ns, const Memory *_Nonnull mem, const Logger *_Nonnull log, Socket sock, const IP_Port *_Nonnull ip_port, Net_Err_Connect *_Nonnull err);

/** @brief High-level getaddrinfo implementation.
 *
 * Given node, which identifies an Internet host, `net_getipport()` fills an array
 * with one or more IP_Port structures, each of which contains an Internet
 * address that can be specified by calling `net_connect()`, the port is ignored.
 *
 * Skip all addresses with socktype != type (use type = -1 to get all addresses)
 * To correctly deallocate array memory use `net_freeipport()`.
 *
 * @param ns Network object.
 * @param mem Memory allocator.
 * @param node The node parameter identifies the host or service on which to connect.
 * @param[out] res An array of IP_Port structures will be allocated into this pointer.
 * @param tox_type The type of socket to use (stream or datagram), only relevant for DNS lookups.
 * @param dns_enabled If false, DNS resolution is skipped, when passed a hostname, this function will return an error.
 *
 * @return number of elements in res array.
 * @retval 0 if res array empty.
 * @retval -1 on error.
 */
int32_t net_getipport(const Network *_Nonnull ns, const Memory *_Nonnull mem, const char *_Nonnull node, IP_Port *_Nullable *_Nonnull res, int tox_type, bool dns_enabled);

/** Deallocates memory allocated by net_getipport */
void net_freeipport(const Memory *_Nonnull mem, IP_Port *_Nullable ip_ports);
bool bin_pack_ip_port(Bin_Pack *_Nonnull bp, const Logger *_Nonnull logger, const IP_Port *_Nonnull ip_port);

/** @brief Pack an IP_Port structure into data of max size length.
 *
 * Packed_length is the offset of data currently packed.
 *
 * @return size of packed IP_Port data on success.
 * @retval -1 on failure.
 */
int pack_ip_port(const Logger *_Nonnull logger, uint8_t *_Nonnull data, uint16_t length, const IP_Port *_Nonnull ip_port);

/** @brief Unpack IP_Port structure from data of max size length into ip_port.
 *
 * len_processed is the offset of data currently unpacked.
 *
 * @return size of unpacked ip_port on success.
 * @retval -1 on failure.
 */
int unpack_ip_port(IP_Port *_Nonnull ip_port, const uint8_t *_Nonnull data, uint16_t length, bool tcp_enabled);

/**
 * @return true on success, false on failure.
 */
bool bind_to_port(const Network *_Nonnull ns, Socket sock, Family family, uint16_t port);

/** @brief Get the last networking error code.
 *
 * Similar to Unix's errno, but cross-platform, as not all platforms use errno
 * to indicate networking errors.
 *
 * Note that different platforms may return different codes for the same error,
 * so you likely shouldn't be checking the value returned by this function
 * unless you know what you are doing, you likely just want to use it in
 * combination with `net_strerror()` to print the error.
 *
 * return platform-dependent network error code, if any.
 */
int net_error(void);

#define NET_STRERROR_SIZE 256

/** @brief Contains a null terminated formatted error message.
 *
 * This struct should not contain more than at most the 2 fields.
 */
typedef struct Net_Strerror {
    char     data[NET_STRERROR_SIZE];
    uint16_t size;
} Net_Strerror;

/** @brief Get a text explanation for the error code from `net_error()`.
 *
 * @param error The error code to get a string for.
 * @param buf The struct to store the error message in (usually on stack).
 *
 * @return pointer to a NULL-terminated string describing the error code.
 */
char *_Nonnull net_strerror(int error, Net_Strerror *_Nonnull buf);

/** @brief Initialize networking.
 * Bind to ip and port.
 * ip must be in network order EX: 127.0.0.1 = (7F000001).
 * port is in host byte order (this means don't worry about it).
 *
 * @return Networking_Core object if no problems
 * @retval NULL if there are problems.
 *
 * If error is non NULL it is set to 0 if no issues, 1 if socket related error, 2 if other.
 */
Networking_Core *_Nullable new_networking_ex(
    const Logger *_Nonnull log, const Memory *_Nonnull mem, const Network *_Nonnull ns, const IP *_Nonnull ip,
    uint16_t port_from, uint16_t port_to, unsigned int *_Nullable error);
Networking_Core *_Nullable new_networking_no_udp(const Logger *_Nonnull log, const Memory *_Nonnull mem, const Network *_Nonnull ns);

/** Function to cleanup networking stuff (doesn't do much right now). */
void kill_networking(Networking_Core *_Nullable net);
/** @brief Returns a pointer to the network net_profile object associated with `net`.
 *
 * Returns null if `net` is null.
 */
const Net_Profile *_Nullable net_get_net_profile(const Networking_Core *_Nonnull net);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXCORE_NETWORK_H */
