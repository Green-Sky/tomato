#include "./tox_av.hpp"

#include <cassert>

#include <cstdint>
#include <iostream>

// https://almogfx.bandcamp.com/track/crushed-w-cassade

ToxAVI::ToxAVI(Tox* tox) : _tox(tox) {
	Toxav_Err_New err_new {TOXAV_ERR_NEW_OK};
	_tox_av = toxav_new(_tox, &err_new);
	// TODO: throw
	assert(err_new == TOXAV_ERR_NEW_OK);

	toxav_callback_call(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_call(friend_number, audio_enabled, video_enabled);
		},
		this
	);
	toxav_callback_call_state(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number, uint32_t state, void *user_data) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_call_state(friend_number, state);
		},
		this
	);
	toxav_callback_audio_bit_rate(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number, uint32_t audio_bit_rate, void *user_data) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_audio_bit_rate(friend_number, audio_bit_rate);
		},
		this
	);
	toxav_callback_video_bit_rate(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number, uint32_t video_bit_rate, void *user_data) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_video_bit_rate(friend_number, video_bit_rate);
		},
		this
	);
	toxav_callback_audio_receive_frame(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate, void *user_data) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_audio_receive_frame(friend_number, pcm, sample_count, channels, sampling_rate);
		},
		this
	);
	toxav_callback_video_receive_frame(
		_tox_av,
		+[](ToxAV*, uint32_t friend_number,
			uint16_t width, uint16_t height,
			const uint8_t y[/*! max(width, abs(ystride)) * height */],
			const uint8_t u[/*! max(width/2, abs(ustride)) * (height/2) */],
			const uint8_t v[/*! max(width/2, abs(vstride)) * (height/2) */],
			int32_t ystride, int32_t ustride, int32_t vstride,
			void *user_data
		) {
			assert(user_data != nullptr);
			static_cast<ToxAVI*>(user_data)->cb_video_receive_frame(friend_number, width, height, y, u, v, ystride, ustride, vstride);
		},
		this
	);
}

ToxAVI::~ToxAVI(void) {
	toxav_kill(_tox_av);
}

uint32_t ToxAVI::toxavIterationInterval(void) const {
	return toxav_iteration_interval(_tox_av);
}

void ToxAVI::toxavIterate(void) {
	toxav_iterate(_tox_av);

	dispatch(
		ToxAV_Event::iterate_audio,
		Events::IterateAudio{
		}
	);
	dispatch(
		ToxAV_Event::iterate_video,
		Events::IterateVideo{
		}
	);
}

uint32_t ToxAVI::toxavAudioIterationInterval(void) const {
	return toxav_audio_iteration_interval(_tox_av);
}

void ToxAVI::toxavAudioIterate(void) {
	toxav_audio_iterate(_tox_av);

	dispatch(
		ToxAV_Event::iterate_audio,
		Events::IterateAudio{
		}
	);
}

uint32_t ToxAVI::toxavVideoIterationInterval(void) const {
	return toxav_video_iteration_interval(_tox_av);
}

void ToxAVI::toxavVideoIterate(void) {
	toxav_video_iterate(_tox_av);

	dispatch(
		ToxAV_Event::iterate_video,
		Events::IterateVideo{
		}
	);
}

Toxav_Err_Call ToxAVI::toxavCall(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate) {
	Toxav_Err_Call err {TOXAV_ERR_CALL_OK};
	toxav_call(_tox_av, friend_number, audio_bit_rate, video_bit_rate, &err);
	return err;
}

Toxav_Err_Answer ToxAVI::toxavAnswer(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate) {
	Toxav_Err_Answer err {TOXAV_ERR_ANSWER_OK};
	toxav_answer(_tox_av, friend_number, audio_bit_rate, video_bit_rate, &err);
	return err;
}

Toxav_Err_Call_Control ToxAVI::toxavCallControl(uint32_t friend_number, Toxav_Call_Control control) {
	Toxav_Err_Call_Control err {TOXAV_ERR_CALL_CONTROL_OK};
	toxav_call_control(_tox_av, friend_number, control, &err);
	return err;
}

Toxav_Err_Send_Frame ToxAVI::toxavAudioSendFrame(uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate) {
	Toxav_Err_Send_Frame err {TOXAV_ERR_SEND_FRAME_OK};
	toxav_audio_send_frame(_tox_av, friend_number, pcm, sample_count, channels, sampling_rate, &err);
	return err;
}

Toxav_Err_Bit_Rate_Set ToxAVI::toxavAudioSetBitRate(uint32_t friend_number, uint32_t bit_rate) {
	Toxav_Err_Bit_Rate_Set err {TOXAV_ERR_BIT_RATE_SET_OK};
	toxav_audio_set_bit_rate(_tox_av, friend_number, bit_rate, &err);
	return err;
}

Toxav_Err_Send_Frame ToxAVI::toxavVideoSendFrame(uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t y[], const uint8_t u[], const uint8_t v[]) {
	Toxav_Err_Send_Frame err {TOXAV_ERR_SEND_FRAME_OK};
	toxav_video_send_frame(_tox_av, friend_number, width, height, y, u, v, &err);
	return err;
}

Toxav_Err_Bit_Rate_Set ToxAVI::toxavVideoSetBitRate(uint32_t friend_number, uint32_t bit_rate) {
	Toxav_Err_Bit_Rate_Set err {TOXAV_ERR_BIT_RATE_SET_OK};
	toxav_video_set_bit_rate(_tox_av, friend_number, bit_rate, &err);
	return err;
}

void ToxAVI::cb_call(uint32_t friend_number, bool audio_enabled, bool video_enabled) {
	std::cerr << "TOXAV: receiving call f:" << friend_number << " a:" << audio_enabled << " v:" << video_enabled << "\n";
	//Toxav_Err_Answer err_answer { TOXAV_ERR_ANSWER_OK };
	//toxav_answer(_tox_av, friend_number, 0, 0, &err_answer);
	//if (err_answer != TOXAV_ERR_ANSWER_OK) {
	//    std::cerr << "!!!!!!!! answer failed " << err_answer << "\n";
	//}

	dispatch(
		ToxAV_Event::friend_call,
		Events::FriendCall{
			friend_number,
			audio_enabled,
			video_enabled,
		}
	);
}

void ToxAVI::cb_call_state(uint32_t friend_number, uint32_t state) {
	//ToxAVFriendCallState w_state{state};

	//w_state.is_error();

	std::cerr << "TOXAV: call state f:" << friend_number << " s:" << state << "\n";

	dispatch(
		ToxAV_Event::friend_call_state,
		Events::FriendCallState{
			friend_number,
			state,
		}
	);
}

void ToxAVI::cb_audio_bit_rate(uint32_t friend_number, uint32_t audio_bit_rate) {
	std::cerr << "TOXAV: audio bitrate f:" << friend_number << " abr:" << audio_bit_rate << "\n";

	dispatch(
		ToxAV_Event::friend_audio_bitrate,
		Events::FriendAudioBitrate{
			friend_number,
			audio_bit_rate,
		}
	);
}

void ToxAVI::cb_video_bit_rate(uint32_t friend_number, uint32_t video_bit_rate) {
	std::cerr << "TOXAV: video bitrate f:" << friend_number << " vbr:" << video_bit_rate << "\n";

	dispatch(
		ToxAV_Event::friend_video_bitrate,
		Events::FriendVideoBitrate{
			friend_number,
			video_bit_rate,
		}
	);
}

void ToxAVI::cb_audio_receive_frame(uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate) {
	//std::cerr << "TOXAV: audio frame f:" <<  friend_number << " sc:" << sample_count << " ch:" << (int)channels << " sr:" << sampling_rate << "\n";

	dispatch(
		ToxAV_Event::friend_audio_frame,
		Events::FriendAudioFrame{
			friend_number,
			Span<int16_t>(pcm, sample_count*channels), // TODO: is sample count *ch or /ch?
			channels,
			sampling_rate,
		}
	);
}

void ToxAVI::cb_video_receive_frame(
	uint32_t friend_number,
	uint16_t width, uint16_t height,
	const uint8_t y[/*! max(width, abs(ystride)) * height */],
	const uint8_t u[/*! max(width/2, abs(ustride)) * (height/2) */],
	const uint8_t v[/*! max(width/2, abs(vstride)) * (height/2) */],
	int32_t ystride, int32_t ustride, int32_t vstride
) {
	//std::cerr << "TOXAV: video frame f:" <<  friend_number << " w:" << width << " h:" << height << "\n";

	dispatch(
		ToxAV_Event::friend_video_frame,
		Events::FriendVideoFrame{
			friend_number,
			width,
			height,
			Span<uint8_t>(y, std::max<int64_t>(width, std::abs(ystride)) * height),
			Span<uint8_t>(u, std::max<int64_t>(width/2, std::abs(ustride)) * (height/2)),
			Span<uint8_t>(v, std::max<int64_t>(width/2, std::abs(vstride)) * (height/2)),
			ystride,
			ustride,
			vstride,
		}
	);
}

