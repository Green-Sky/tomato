#pragma once

#include <solanaceae/util/span.hpp>
#include <solanaceae/util/event_provider.hpp>

#include <tox/toxav.h>

namespace /*toxav*/ Events {

	struct FriendCall {
		uint32_t friend_number;
		bool audio_enabled;
		bool video_enabled;
	};

	struct FriendCallState {
		uint32_t friend_number;
		uint32_t state;
	};

	struct FriendAudioBitrate {
		uint32_t friend_number;
		uint32_t audio_bit_rate;
	};

	struct FriendVideoBitrate {
		uint32_t friend_number;
		uint32_t video_bit_rate;
	};

	struct FriendAudioFrame {
		uint32_t friend_number;

		Span<int16_t> pcm;
		//size_t sample_count;
		uint8_t channels;
		uint32_t sampling_rate;
	};

	struct FriendVideoFrame {
		uint32_t friend_number;

		uint16_t width;
		uint16_t height;
		//const uint8_t y[[>! max(width, abs(ystride)) * height <]];
		//const uint8_t u[[>! max(width/2, abs(ustride)) * (height/2) <]];
		//const uint8_t v[[>! max(width/2, abs(vstride)) * (height/2) <]];
		// mdspan would be nice here
		// bc of the stride, span might be larger than the actual data it contains
		Span<uint8_t> y;
		Span<uint8_t> u;
		Span<uint8_t> v;
		int32_t ystride;
		int32_t ustride;
		int32_t vstride;
	};

	// event fired on a/av thread every iterate
	struct IterateAudio {
		//float time_delta;
	};

	// event fired on v/av thread every iterate
	struct IterateVideo {
		//float time_delta;
	};

} // Event

enum class ToxAV_Event : uint32_t {
	friend_call,
	friend_call_state,
	friend_audio_bitrate,
	friend_video_bitrate,
	friend_audio_frame,
	friend_video_frame,

	iterate_audio,
	iterate_video,

	MAX
};

struct ToxAVEventI {
	using enumType = ToxAV_Event;

	virtual ~ToxAVEventI(void) {}

	virtual bool onEvent(const Events::FriendCall&) { return false; }
	virtual bool onEvent(const Events::FriendCallState&) { return false; }
	virtual bool onEvent(const Events::FriendAudioBitrate&) { return false; }
	virtual bool onEvent(const Events::FriendVideoBitrate&) { return false; }
	virtual bool onEvent(const Events::FriendAudioFrame&) { return false; }
	virtual bool onEvent(const Events::FriendVideoFrame&) { return false; }
	virtual bool onEvent(const Events::IterateAudio&) { return false; }
	virtual bool onEvent(const Events::IterateVideo&) { return false; }
};
using ToxAVEventProviderI = EventProviderI<ToxAVEventI>;

// TODO: seperate out implementation from interface
struct ToxAVI : public ToxAVEventProviderI {
	// tox and toxav internally are mutex protected
	// BUT only if "experimental_thread_safety" is enabled
	Tox* _tox = nullptr;
	ToxAV* _tox_av = nullptr;

	static constexpr const char* version {"0"};

	ToxAVI(Tox* tox);
	virtual ~ToxAVI(void);

	// NOTE: interval timers are only interesting for receiving streams
	// if we are only sending, it will always report 195ms

	// interface
	// if iterate is called on a different thread, it will fire events there
	uint32_t toxavIterationInterval(void) const;
	void toxavIterate(void);

	uint32_t toxavAudioIterationInterval(void) const;
	void toxavAudioIterate(void);
	uint32_t toxavVideoIterationInterval(void) const;
	void toxavVideoIterate(void);

	Toxav_Err_Call toxavCall(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);
	Toxav_Err_Answer toxavAnswer(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);
	Toxav_Err_Call_Control toxavCallControl(uint32_t friend_number, Toxav_Call_Control control);
	Toxav_Err_Send_Frame toxavAudioSendFrame(uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate);
	Toxav_Err_Bit_Rate_Set toxavAudioSetBitRate(uint32_t friend_number, uint32_t bit_rate);
	Toxav_Err_Send_Frame toxavVideoSendFrame(uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t y[/*! height * width */], const uint8_t u[/*! height/2 * width/2 */], const uint8_t v[/*! height/2 * width/2 */]);
	Toxav_Err_Bit_Rate_Set toxavVideoSetBitRate(uint32_t friend_number, uint32_t bit_rate);

//int32_t toxav_add_av_groupchat(Tox *tox, toxav_audio_data_cb *audio_callback, void *userdata);
//int32_t toxav_join_av_groupchat(Tox *tox, uint32_t friendnumber, const uint8_t data[], uint16_t length, toxav_audio_data_cb *audio_callback, void *userdata);
//int32_t toxav_group_send_audio(Tox *tox, uint32_t groupnumber, const int16_t pcm[], uint32_t samples, uint8_t channels, uint32_t sample_rate);
//int32_t toxav_groupchat_enable_av(Tox *tox, uint32_t groupnumber, toxav_audio_data_cb *audio_callback, void *userdata);
//int32_t toxav_groupchat_disable_av(Tox *tox, uint32_t groupnumber);
//bool toxav_groupchat_av_enabled(Tox *tox, uint32_t groupnumber);



	// toxav callbacks
	void cb_call(uint32_t friend_number, bool audio_enabled, bool video_enabled);
	void cb_call_state(uint32_t friend_number, uint32_t state);
	void cb_audio_bit_rate(uint32_t friend_number, uint32_t audio_bit_rate);
	void cb_video_bit_rate(uint32_t friend_number, uint32_t video_bit_rate);
	void cb_audio_receive_frame(uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate);
	void cb_video_receive_frame(
		uint32_t friend_number,
		uint16_t width, uint16_t height,
		const uint8_t y[/*! max(width, abs(ystride)) * height */],
		const uint8_t u[/*! max(width/2, abs(ustride)) * (height/2) */],
		const uint8_t v[/*! max(width/2, abs(vstride)) * (height/2) */],
		int32_t ystride, int32_t ustride, int32_t vstride
	);
};

struct ToxAVFriendCallState final {
	const uint32_t state {TOXAV_FRIEND_CALL_STATE_NONE};

	[[nodiscard]] bool is_error(void) const { return state & TOXAV_FRIEND_CALL_STATE_ERROR; }
	[[nodiscard]] bool is_finished(void) const { return state & TOXAV_FRIEND_CALL_STATE_FINISHED; }
	[[nodiscard]] bool is_sending_a(void) const { return state & TOXAV_FRIEND_CALL_STATE_SENDING_A; }
	[[nodiscard]] bool is_sending_v(void) const { return state & TOXAV_FRIEND_CALL_STATE_SENDING_V; }
	[[nodiscard]] bool is_accepting_a(void) const { return state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_A; }
	[[nodiscard]] bool is_accepting_v(void) const { return state & TOXAV_FRIEND_CALL_STATE_ACCEPTING_V; }
};

