cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(TOX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/c-toxcore/")

# TODO: shared
add_library(toxcore STATIC
	${TOX_DIR}third_party/cmp/cmp.c
	${TOX_DIR}third_party/cmp/cmp.h

	${TOX_DIR}toxcore/announce.c
	${TOX_DIR}toxcore/announce.h
	${TOX_DIR}toxcore/bin_pack.c
	${TOX_DIR}toxcore/bin_pack.h
	${TOX_DIR}toxcore/bin_unpack.c
	${TOX_DIR}toxcore/bin_unpack.h
	${TOX_DIR}toxcore/ccompat.c
	${TOX_DIR}toxcore/ccompat.h
	${TOX_DIR}toxcore/crypto_core.c
	${TOX_DIR}toxcore/crypto_core.h
	${TOX_DIR}toxcore/crypto_core_pack.c
	${TOX_DIR}toxcore/crypto_core_pack.h
	${TOX_DIR}toxcore/DHT.c
	${TOX_DIR}toxcore/DHT.h
	${TOX_DIR}toxcore/events/conference_connected.c
	${TOX_DIR}toxcore/events/conference_invite.c
	${TOX_DIR}toxcore/events/conference_message.c
	${TOX_DIR}toxcore/events/conference_peer_list_changed.c
	${TOX_DIR}toxcore/events/conference_peer_name.c
	${TOX_DIR}toxcore/events/conference_title.c
	${TOX_DIR}toxcore/events/dht_get_nodes_response.c
	${TOX_DIR}toxcore/events/events_alloc.c
	${TOX_DIR}toxcore/events/events_alloc.h
	${TOX_DIR}toxcore/events/file_chunk_request.c
	${TOX_DIR}toxcore/events/file_recv.c
	${TOX_DIR}toxcore/events/file_recv_chunk.c
	${TOX_DIR}toxcore/events/file_recv_control.c
	${TOX_DIR}toxcore/events/friend_connection_status.c
	${TOX_DIR}toxcore/events/friend_lossless_packet.c
	${TOX_DIR}toxcore/events/friend_lossy_packet.c
	${TOX_DIR}toxcore/events/friend_message.c
	${TOX_DIR}toxcore/events/friend_name.c
	${TOX_DIR}toxcore/events/friend_read_receipt.c
	${TOX_DIR}toxcore/events/friend_request.c
	${TOX_DIR}toxcore/events/friend_status.c
	${TOX_DIR}toxcore/events/friend_status_message.c
	${TOX_DIR}toxcore/events/friend_typing.c
	${TOX_DIR}toxcore/events/self_connection_status.c
	${TOX_DIR}toxcore/events/group_custom_packet.c
	${TOX_DIR}toxcore/events/group_custom_private_packet.c
	${TOX_DIR}toxcore/events/group_invite.c
	${TOX_DIR}toxcore/events/group_join_fail.c
	${TOX_DIR}toxcore/events/group_message.c
	${TOX_DIR}toxcore/events/group_moderation.c
	${TOX_DIR}toxcore/events/group_password.c
	${TOX_DIR}toxcore/events/group_peer_exit.c
	${TOX_DIR}toxcore/events/group_peer_join.c
	${TOX_DIR}toxcore/events/group_peer_limit.c
	${TOX_DIR}toxcore/events/group_peer_name.c
	${TOX_DIR}toxcore/events/group_peer_status.c
	${TOX_DIR}toxcore/events/group_privacy_state.c
	${TOX_DIR}toxcore/events/group_private_message.c
	${TOX_DIR}toxcore/events/group_self_join.c
	${TOX_DIR}toxcore/events/group_topic.c
	${TOX_DIR}toxcore/events/group_topic_lock.c
	${TOX_DIR}toxcore/events/group_voice_state.c
	${TOX_DIR}toxcore/forwarding.c
	${TOX_DIR}toxcore/forwarding.h
	${TOX_DIR}toxcore/friend_connection.c
	${TOX_DIR}toxcore/friend_connection.h
	${TOX_DIR}toxcore/friend_requests.c
	${TOX_DIR}toxcore/friend_requests.h
	${TOX_DIR}toxcore/group.c
	${TOX_DIR}toxcore/group.h
	${TOX_DIR}toxcore/group_announce.c
	${TOX_DIR}toxcore/group_announce.h
	${TOX_DIR}toxcore/group_moderation.c
	${TOX_DIR}toxcore/group_moderation.h
	${TOX_DIR}toxcore/group_chats.c
	${TOX_DIR}toxcore/group_chats.h
	${TOX_DIR}toxcore/group_common.h
	${TOX_DIR}toxcore/group_connection.c
	${TOX_DIR}toxcore/group_connection.h
	${TOX_DIR}toxcore/group_onion_announce.c
	${TOX_DIR}toxcore/group_onion_announce.h
	${TOX_DIR}toxcore/group_pack.c
	${TOX_DIR}toxcore/group_pack.h
	${TOX_DIR}toxcore/LAN_discovery.c
	${TOX_DIR}toxcore/LAN_discovery.h
	${TOX_DIR}toxcore/list.c
	${TOX_DIR}toxcore/list.h
	${TOX_DIR}toxcore/logger.c
	${TOX_DIR}toxcore/logger.h
	${TOX_DIR}toxcore/Messenger.c
	${TOX_DIR}toxcore/Messenger.h
	${TOX_DIR}toxcore/mem.c
	${TOX_DIR}toxcore/mem.h
	${TOX_DIR}toxcore/mono_time.c
	${TOX_DIR}toxcore/mono_time.h
	${TOX_DIR}toxcore/net_crypto.c
	${TOX_DIR}toxcore/net_crypto.h
	${TOX_DIR}toxcore/network.c
	${TOX_DIR}toxcore/network.h
	${TOX_DIR}toxcore/onion_announce.c
	${TOX_DIR}toxcore/onion_announce.h
	${TOX_DIR}toxcore/onion.c
	${TOX_DIR}toxcore/onion_client.c
	${TOX_DIR}toxcore/onion_client.h
	${TOX_DIR}toxcore/onion.h
	${TOX_DIR}toxcore/ping_array.c
	${TOX_DIR}toxcore/ping_array.h
	${TOX_DIR}toxcore/ping.c
	${TOX_DIR}toxcore/ping.h
	${TOX_DIR}toxcore/shared_key_cache.c
	${TOX_DIR}toxcore/shared_key_cache.h
	${TOX_DIR}toxcore/state.c
	${TOX_DIR}toxcore/state.h
	${TOX_DIR}toxcore/TCP_client.c
	${TOX_DIR}toxcore/TCP_client.h
	${TOX_DIR}toxcore/TCP_common.c
	${TOX_DIR}toxcore/TCP_common.h
	${TOX_DIR}toxcore/TCP_connection.c
	${TOX_DIR}toxcore/TCP_connection.h
	${TOX_DIR}toxcore/TCP_server.c
	${TOX_DIR}toxcore/TCP_server.h
	${TOX_DIR}toxcore/timed_auth.c
	${TOX_DIR}toxcore/timed_auth.h
	${TOX_DIR}toxcore/tox_api.c
	${TOX_DIR}toxcore/tox.c
	${TOX_DIR}toxcore/tox_dispatch.c
	${TOX_DIR}toxcore/tox_dispatch.h
	${TOX_DIR}toxcore/tox_event.c
	${TOX_DIR}toxcore/tox_event.h
	${TOX_DIR}toxcore/tox_events.c
	${TOX_DIR}toxcore/tox_events.h
	${TOX_DIR}toxcore/tox.h
	${TOX_DIR}toxcore/tox_private.c
	${TOX_DIR}toxcore/tox_private.h
	${TOX_DIR}toxcore/tox_pack.h
	${TOX_DIR}toxcore/tox_pack.c
	${TOX_DIR}toxcore/tox_unpack.c
	${TOX_DIR}toxcore/tox_unpack.h
	${TOX_DIR}toxcore/util.c
	${TOX_DIR}toxcore/util.h

	${TOX_DIR}toxencryptsave/defines.h
	${TOX_DIR}toxencryptsave/toxencryptsave.c
	${TOX_DIR}toxencryptsave/toxencryptsave.h
)

# HACK: "install" api headers into self
# this is dirty, should be binary dir
# TODO: add the others
configure_file(
	${TOX_DIR}toxcore/tox.h
	${TOX_DIR}tox/tox.h
	@ONLY
)
configure_file(
	${TOX_DIR}toxcore/tox_events.h
	${TOX_DIR}tox/tox_events.h
	@ONLY
)
configure_file(
	${TOX_DIR}toxcore/tox_private.h
	${TOX_DIR}tox/tox_private.h
	@ONLY
)

target_include_directories(toxcore PRIVATE "${TOX_DIR}toxcore")
target_include_directories(toxcore PUBLIC "${TOX_DIR}")

#target_compile_definitions(toxcore PUBLIC USE_IPV6=1)
#target_compile_definitions(toxcore PUBLIC MIN_LOGGER_LEVEL=LOGGER_LEVEL_DEBUG)
#target_compile_definitions(toxcore PUBLIC MIN_LOGGER_LEVEL=LOGGER_LEVEL_INFO)
target_compile_definitions(toxcore PUBLIC MIN_LOGGER_LEVEL=LOGGER_LEVEL_TRACE)

find_package(unofficial-sodium CONFIG QUIET)
find_package(sodium QUIET)
if(unofficial-sodium_FOUND) # vcpkg
	if(TARGET unofficial-sodium::sodium)
		target_link_libraries(toxcore unofficial-sodium::sodium)
	endif()
	if(TARGET unofficial-sodium::sodium_config_public)
		target_link_libraries(toxcore unofficial-sodium::sodium_config_public)
	endif()
elseif(sodium_FOUND)
	target_link_libraries(toxcore sodium)
else()
	message(SEND_ERROR "missing libsodium")
endif()

if(WIN32)
	target_link_libraries(toxcore ws2_32 iphlpapi)
endif()

find_package(pthreads QUIET)
if(TARGET PThreads4W::PThreads4W)
	target_link_libraries(toxcore PThreads4W::PThreads4W)
else()
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	target_link_libraries(toxcore Threads::Threads)
endif()

add_executable(DHT_Bootstrap EXCLUDE_FROM_ALL
	${TOX_DIR}other/DHT_bootstrap.c
	${TOX_DIR}other/bootstrap_node_packets.h
	${TOX_DIR}other/bootstrap_node_packets.c
	${TOX_DIR}testing/misc_tools.h
	${TOX_DIR}testing/misc_tools.c
)
target_link_libraries(DHT_Bootstrap toxcore)

