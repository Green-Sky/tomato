#include "video.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "../toxcore/logger.h"
#include "../toxcore/mono_time.h"
#include "../toxcore/network.h"
#include "../toxcore/os_memory.h"
#include "rtp.h"

namespace {

struct VideoTimeMock {
    uint64_t t;
};

uint64_t video_mock_time_cb(void *ud) { return static_cast<VideoTimeMock *>(ud)->t; }

void test_logger_cb(void *context, Logger_Level level, const char *file, uint32_t line,
    const char *func, const char *message, void *userdata)
{
    (void)context;
    (void)userdata;
    const char *level_str = "UNKNOWN";
    switch (level) {
    case LOGGER_LEVEL_TRACE:
        level_str = "TRACE";
        break;
    case LOGGER_LEVEL_DEBUG:
        level_str = "DEBUG";
        break;
    case LOGGER_LEVEL_INFO:
        level_str = "INFO";
        break;
    case LOGGER_LEVEL_WARNING:
        level_str = "WARN";
        break;
    case LOGGER_LEVEL_ERROR:
        level_str = "ERROR";
        break;
    }
    printf("[%s] %s:%u %s: %s\n", level_str, file, line, func, message);
}

struct VideoTestData {
    uint32_t friend_number = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<uint8_t> y, u, v;
    int32_t ystride = 0, ustride = 0, vstride = 0;

    VideoTestData();
    ~VideoTestData();

    static void receive_frame(uint32_t friend_number, uint16_t width, uint16_t height,
        const uint8_t *y, const uint8_t *u, const uint8_t *v, int32_t ystride, int32_t ustride,
        int32_t vstride, void *user_data)
    {
        auto *self = static_cast<VideoTestData *>(user_data);
        self->friend_number = friend_number;
        self->width = width;
        self->height = height;
        self->ystride = ystride;
        self->ustride = ustride;
        self->vstride = vstride;

        self->y.assign(y, y + static_cast<size_t>(std::abs(ystride)) * height);
        self->u.assign(u, u + static_cast<size_t>(std::abs(ustride)) * (height / 2));
        self->v.assign(v, v + static_cast<size_t>(std::abs(vstride)) * (height / 2));
    }
};

VideoTestData::VideoTestData() = default;
VideoTestData::~VideoTestData() = default;

struct VideoRtpMock {
    RTPSession *recv_session = nullptr;
    std::vector<std::vector<uint8_t>> captured_packets;
    bool auto_forward = true;

    static int send_packet(void *user_data, const uint8_t *data, uint16_t length)
    {
        auto *self = static_cast<VideoRtpMock *>(user_data);
        self->captured_packets.push_back(std::vector<uint8_t>(data, data + length));
        if (self->auto_forward && self->recv_session) {
            rtp_receive_packet(self->recv_session, data, length);
        }
        return 0;
    }

    static int video_cb(const Mono_Time *mono_time, void *cs, RTPMessage *msg)
    {
        return vc_queue_message(mono_time, cs, msg);
    }
};

class VideoTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        const Memory *mem = os_memory();
        log = logger_new(mem);
        logger_callback_log(log, test_logger_cb, nullptr, nullptr);
        tm.t = 1000;
        mono_time = mono_time_new(mem, video_mock_time_cb, &tm);
        mono_time_update(mono_time);
    }

    void TearDown() override
    {
        const Memory *mem = os_memory();
        mono_time_free(mem, mono_time);
        logger_kill(log);
    }

    Logger *log;
    Mono_Time *mono_time;
    VideoTimeMock tm;
};

TEST_F(VideoTest, BasicNewKill)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);
    vc_kill(vc);
}

TEST_F(VideoTest, EncodeDecodeLoop)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    VideoRtpMock rtp_mock;
    RTPSession *send_rtp = rtp_new(log, RTP_TYPE_VIDEO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    RTPSession *recv_rtp = rtp_new(log, RTP_TYPE_VIDEO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    rtp_mock.recv_session = recv_rtp;

    uint16_t width = 320;
    uint16_t height = 240;
    uint32_t bitrate = 500;

    ASSERT_EQ(vc_reconfigure_encoder(vc, bitrate, width, height, -1), 0);

    std::vector<uint8_t> y(width * height, 128);
    std::vector<uint8_t> u((width / 2) * (height / 2), 64);
    std::vector<uint8_t> v((width / 2) * (height / 2), 192);

    ASSERT_EQ(vc_encode(vc, width, height, y.data(), u.data(), v.data(), VC_EFLAG_FORCE_KF), 0);
    vc_increment_frame_counter(vc);

    uint8_t *pkt_data;
    uint32_t pkt_size;
    bool is_keyframe;

    while (vc_get_cx_data(vc, &pkt_data, &pkt_size, &is_keyframe)) {
        int rc = rtp_send_data(log, send_rtp, pkt_data, pkt_size, is_keyframe);
        ASSERT_EQ(rc, 0);
    }

    vc_iterate(vc);

    ASSERT_EQ(data.friend_number, 123u);
    ASSERT_EQ(data.width, width);
    ASSERT_EQ(data.height, height);
    ASSERT_FALSE(data.y.empty());

    rtp_kill(log, send_rtp);
    rtp_kill(log, recv_rtp);
    vc_kill(vc);
}

TEST_F(VideoTest, ReconfigureEncoder)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    // Initial reconfigure
    ASSERT_EQ(vc_reconfigure_encoder(vc, 500, 320, 240, -1), 0);

    // Change bitrate and resolution
    ASSERT_EQ(vc_reconfigure_encoder(vc, 1000, 640, 480, -1), 0);

    std::vector<uint8_t> y(640 * 480, 128);
    std::vector<uint8_t> u(320 * 240, 64);
    std::vector<uint8_t> v(320 * 240, 192);

    ASSERT_EQ(vc_encode(vc, 640, 480, y.data(), u.data(), v.data(), VC_EFLAG_NONE), 0);

    vc_kill(vc);
}

TEST_F(VideoTest, GetLcfd)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    // Default lcfd is 60 in video.c
    EXPECT_EQ(vc_get_lcfd(vc), 60u);

    vc_kill(vc);
}

TEST_F(VideoTest, QueueInvalidMessage)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    VideoRtpMock rtp_mock;
    // Create an audio RTP session but try to queue to video session
    RTPSession *audio_rtp = rtp_new(log, RTP_TYPE_AUDIO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    RTPSession *video_recv_rtp = rtp_new(log, RTP_TYPE_VIDEO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    rtp_mock.recv_session = video_recv_rtp;

    std::vector<uint8_t> dummy_audio(100, 0);
    int rc = rtp_send_data(
        log, audio_rtp, dummy_audio.data(), static_cast<uint32_t>(dummy_audio.size()), false);
    ASSERT_EQ(rc, 0);

    // Iterate should NOT trigger callback because payload type was wrong
    vc_iterate(vc);
    EXPECT_EQ(data.width, 0u);

    rtp_kill(log, audio_rtp);
    rtp_kill(log, video_recv_rtp);
    vc_kill(vc);
}

TEST_F(VideoTest, ReconfigureOptimizations)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    // 1. Reconfigure with same values (should do nothing)
    // vc_new initializes encoder with 800x600 and 5000 bitrate.
    EXPECT_EQ(vc_reconfigure_encoder(vc, 5000, 800, 600, -1), 0);

    // 2. Reconfigure with only bitrate change
    EXPECT_EQ(vc_reconfigure_encoder(vc, 2000, 800, 600, -1), 0);

    // 3. Reconfigure with kf_max_dist > 1 (triggers re-init and kf_max_dist branch)
    EXPECT_EQ(vc_reconfigure_encoder(vc, 2000, 800, 600, 60), 0);

    vc_kill(vc);
}

TEST_F(VideoTest, LcfdAndSpecialPackets)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    VideoRtpMock rtp_mock;
    RTPSession *video_recv_rtp = rtp_new(log, RTP_TYPE_VIDEO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    rtp_mock.recv_session = video_recv_rtp;

    // 1. Test lcfd update
    tm.t += 50;  // Advance time by 50ms
    mono_time_update(mono_time);
    std::vector<uint8_t> dummy_frame(10, 0);
    rtp_send_data(
        log, video_recv_rtp, dummy_frame.data(), static_cast<uint32_t>(dummy_frame.size()), true);

    // lcfd should be updated. Initial linfts was set at vc_new (tm.t=1000).
    // Now tm.t is 1050. t_lcfd = 1050 - 1000 = 50.
    EXPECT_EQ(vc_get_lcfd(vc), 50u);

    // 2. Test lcfd threshold (t_lcfd > 100 should be ignored)
    tm.t += 200;
    mono_time_update(mono_time);
    rtp_send_data(
        log, video_recv_rtp, dummy_frame.data(), static_cast<uint32_t>(dummy_frame.size()), true);
    EXPECT_EQ(vc_get_lcfd(vc), 50u);  // Should still be 50

    // 3. Test dummy packet PT = (RTP_TYPE_VIDEO + 2) % 128
    RTPSession *dummy_rtp = rtp_new(log, (RTP_TYPE_VIDEO + 2), mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    rtp_mock.recv_session = dummy_rtp;
    rtp_send_data(
        log, dummy_rtp, dummy_frame.data(), static_cast<uint32_t>(dummy_frame.size()), false);
    // Should return 0 but do nothing (logged as "Got dummy!")

    // 4. Test GetQueueMutex
    EXPECT_NE(vc_get_queue_mutex(vc), nullptr);

    rtp_kill(log, video_recv_rtp);
    rtp_kill(log, dummy_rtp);
    vc_kill(vc);
}

TEST_F(VideoTest, MultiReconfigureEncode)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    for (int i = 0; i < 5; ++i) {
        uint16_t w = static_cast<uint16_t>(160 + (i * 16));
        uint16_t h = static_cast<uint16_t>(120 + (i * 16));
        std::vector<uint8_t> y(static_cast<size_t>(w) * h, 128);
        std::vector<uint8_t> u((static_cast<size_t>(w) / 2) * (h / 2), 64);
        std::vector<uint8_t> v((static_cast<size_t>(w) / 2) * (h / 2), 192);

        ASSERT_EQ(vc_reconfigure_encoder(vc, 1000, w, h, -1), 0);
        ASSERT_EQ(vc_encode(vc, w, h, y.data(), u.data(), v.data(), VC_EFLAG_NONE), 0);
    }

    vc_kill(vc);
}

TEST_F(VideoTest, NewWithNullMonoTime)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, nullptr, 123, VideoTestData::receive_frame, &data);
    EXPECT_EQ(vc, nullptr);
}

TEST_F(VideoTest, ReconfigureFailDoS)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    // Trigger failure by passing invalid resolution (0)
    // This currently destroys the encoder.
    ASSERT_EQ(vc_reconfigure_encoder(vc, 1000, 0, 0, -1), -1);

    // Attempt to encode. This is expected to crash because vc->encoder is destroyed.
    std::vector<uint8_t> y(320 * 240, 128);
    std::vector<uint8_t> u(160 * 120, 64);
    std::vector<uint8_t> v(160 * 120, 192);
    // This call will crash in the current unfixed code.
    vc_encode(vc, 320, 240, y.data(), u.data(), v.data(), VC_EFLAG_NONE);

    vc_kill(vc);
}

TEST_F(VideoTest, LyingLengthOOB)
{
    VideoTestData data;
    VCSession *vc = vc_new(log, mono_time, 123, VideoTestData::receive_frame, &data);
    ASSERT_NE(vc, nullptr);

    VideoRtpMock rtp_mock;
    RTPSession *recv_rtp = rtp_new(log, RTP_TYPE_VIDEO, mono_time, VideoRtpMock::send_packet,
        &rtp_mock, nullptr, nullptr, nullptr, vc, VideoRtpMock::video_cb);
    rtp_mock.recv_session = recv_rtp;

    // Craft a malicious RTP packet
    uint16_t payload_len = 10;
    uint8_t packet[RTP_HEADER_SIZE + 11];  // +1 for Tox ID
    memset(packet, 0, sizeof(packet));

    // Tox ID
    packet[0] = static_cast<uint8_t>(RTP_TYPE_VIDEO);

    auto pack_u16 = [](uint8_t *p, uint16_t v) {
        p[0] = static_cast<uint8_t>(v >> 8);
        p[1] = static_cast<uint8_t>(v & 0xff);
    };
    auto pack_u32 = [](uint8_t *p, uint32_t v) {
        p[0] = static_cast<uint8_t>(v >> 24);
        p[1] = static_cast<uint8_t>((v >> 16) & 0xff);
        p[2] = static_cast<uint8_t>((v >> 8) & 0xff);
        p[3] = static_cast<uint8_t>(v & 0xff);
    };
    auto pack_u64 = [&](uint8_t *p, uint64_t v) {
        pack_u32(p, static_cast<uint32_t>(v >> 32));
        pack_u32(p + 4, static_cast<uint32_t>(v & 0xffffffff));
    };

    // RTP Header starts at packet[1]
    packet[1] = 2 << 6;  // ve = 2
    packet[2] = static_cast<uint8_t>(RTP_TYPE_VIDEO % 128);

    pack_u16(packet + 3, 1);  // sequnum
    pack_u32(packet + 5, 1000);  // timestamp
    pack_u32(packet + 9, 0x12345678);  // ssrc
    pack_u64(packet + 13, RTP_LARGE_FRAME);  // flags
    pack_u32(packet + 21, 0);  // offset_full
    pack_u32(packet + 25, 1000);  // data_length_full (LYING! Actual is 10)
    pack_u32(packet + 29, 0);  // received_length_full

    // Skip padding fields (11 * 4 = 44 bytes)
    pack_u16(packet + 77, 0);  // offset_lower
    pack_u16(packet + 79, payload_len);  // data_length_lower

    // Send the malicious packet
    rtp_receive_packet(recv_rtp, packet, sizeof(packet));

    // Trigger vc_iterate. This will call vpx_codec_decode with length 1000.
    // This is expected to cause OOB read.
    vc_iterate(vc);

    rtp_kill(log, recv_rtp);
    vc_kill(vc);
}

}  // namespace
