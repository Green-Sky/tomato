/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright © 2016-2025 The TokTok team.
 * Copyright © 2013-2015 Tox project.
 */
#ifndef C_TOXCORE_TOXAV_VIDEO_H
#define C_TOXCORE_TOXAV_VIDEO_H

#include <pthread.h>
#include <stdint.h>

#include "../toxcore/logger.h"
#include "../toxcore/mono_time.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void vc_video_receive_frame_cb(uint32_t friend_number, uint16_t width, uint16_t height,
                                       const uint8_t *y, const uint8_t *u, const uint8_t *v,
                                       int32_t ystride, int32_t ustride, int32_t vstride,
                                       void *user_data);

typedef struct VCSession VCSession;

#define VC_EFLAG_NONE 0
#define VC_EFLAG_FORCE_KF (1 << 0)

struct RTPMessage;

VCSession *vc_new(const Logger *log, const Mono_Time *mono_time, uint32_t friend_number,
                  vc_video_receive_frame_cb *cb, void *user_data);
void vc_kill(VCSession *vc);
void vc_iterate(VCSession *vc);

int vc_queue_message(const Mono_Time *mono_time, void *cs, struct RTPMessage *msg);
int vc_reconfigure_encoder(VCSession *vc, uint32_t bit_rate, uint16_t width, uint16_t height, int16_t kf_max_dist);

int vc_encode(VCSession *vc, uint16_t width, uint16_t height, const uint8_t *y,
              const uint8_t *u, const uint8_t *v, int encode_flags);

int vc_get_cx_data(VCSession *vc, uint8_t **data, uint32_t *size, bool *is_keyframe);
uint32_t vc_get_lcfd(const VCSession *vc);
pthread_mutex_t *vc_get_queue_mutex(VCSession *vc);
void vc_increment_frame_counter(VCSession *vc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* C_TOXCORE_TOXAV_VIDEO_H */
