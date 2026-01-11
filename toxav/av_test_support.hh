#ifndef TOXAV_AV_TEST_SUPPORT_H
#define TOXAV_AV_TEST_SUPPORT_H

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "../toxcore/logger.h"
#include "../toxcore/mono_time.h"
#include "audio.h"
#include "rtp.h"
#include "video.h"

// Mock Time
struct MockTime {
    uint64_t t = 1000;
};
uint64_t mock_time_cb(void *ud);

// RTP Mock
struct RtpMock {
    RTPSession *recv_session = nullptr;
    std::vector<std::vector<uint8_t>> captured_packets;
    bool auto_forward = true;
    bool capture_packets = true;
    bool store_last_packet_only = false;

    static int send_packet(void *user_data, const uint8_t *data, uint16_t length);
    static int audio_cb(const Mono_Time *mono_time, void *cs, RTPMessage *msg);
    static int video_cb(const Mono_Time *mono_time, void *cs, RTPMessage *msg);
    static int noop_cb(const Mono_Time *mono_time, void *cs, RTPMessage *msg);
};

// Audio Helpers
void fill_audio_frame(uint32_t sampling_rate, uint8_t channels, int frame_index,
    size_t sample_count, std::vector<int16_t> &pcm);
void fill_silent_frame(uint8_t channels, size_t sample_count, std::vector<int16_t> &pcm);

struct AudioTestData {
    uint32_t friend_number = 0;
    std::vector<int16_t> last_pcm;
    size_t sample_count = 0;
    uint8_t channels = 0;
    uint32_t sampling_rate = 0;

    AudioTestData();
    ~AudioTestData();

    static void receive_frame(uint32_t friend_number, const int16_t *pcm, size_t sample_count,
        uint8_t channels, uint32_t sampling_rate, void *user_data);
};

// Video Helpers
void fill_video_frame(uint16_t width, uint16_t height, int frame_index, std::vector<uint8_t> &y,
    std::vector<uint8_t> &u, std::vector<uint8_t> &v);
double calculate_video_mse(uint16_t width, uint16_t height, int32_t ystride,
    const std::vector<uint8_t> &y_recv, const std::vector<uint8_t> &y_orig);

// Video Test Data Helper
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
        int32_t vstride, void *user_data);

    double calculate_mse(const std::vector<uint8_t> &y_orig) const;
};

// Common Test Fixture
class AvTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    Logger *log = nullptr;
    Mono_Time *mono_time = nullptr;
    MockTime tm;
};

#endif  // TOXAV_AV_TEST_SUPPORT_H
