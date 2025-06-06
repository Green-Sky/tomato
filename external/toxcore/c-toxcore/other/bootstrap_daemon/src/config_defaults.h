/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2014-2016 Tox project.
 */

/*
 * Tox DHT bootstrap daemon.
 * Default config options for when they are missing in the config file.
 */
#ifndef C_TOXCORE_OTHER_BOOTSTRAP_DAEMON_SRC_CONFIG_DEFAULTS_H
#define C_TOXCORE_OTHER_BOOTSTRAP_DAEMON_SRC_CONFIG_DEFAULTS_H

#include <stdbool.h>

#include "global.h"

#define DEFAULT_PID_FILE_PATH         "tox-bootstrapd.pid"
#define DEFAULT_KEYS_FILE_PATH        "tox-bootstrapd.keys"
#define DEFAULT_PORT                  33445
#define DEFAULT_ENABLE_IPV6           true
#define DEFAULT_ENABLE_IPV4_FALLBACK  true
#define DEFAULT_ENABLE_LAN_DISCOVERY  true
#define DEFAULT_ENABLE_TCP_RELAY      true
#define DEFAULT_TCP_RELAY_PORTS       443, 3389, 33445 // comma-separated list of ports
#define DEFAULT_ENABLE_MOTD           true
#define DEFAULT_MOTD                  DAEMON_NAME

#endif // C_TOXCORE_OTHER_BOOTSTRAP_DAEMON_SRC_CONFIG_DEFAULTS_H
