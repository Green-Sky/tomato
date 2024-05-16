#include "./tox_av.hpp"

#include <cassert>

// https://almogfx.bandcamp.com/track/crushed-w-cassade

ToxAV::ToxAV(Tox* tox) : _tox(tox) {
	Toxav_Err_New err_new {TOXAV_ERR_NEW_OK};
	_tox_av = toxav_new(_tox, &err_new);
	// TODO: throw
	assert(err_new == TOXAV_ERR_NEW_OK);
}
ToxAV::~ToxAV(void) {
	toxav_kill(_tox_av);
}

uint32_t ToxAV::toxavIterationInterval(void) const {
	return toxav_iteration_interval(_tox_av);
}

void ToxAV::toxavIterate(void) {
	toxav_iterate(_tox_av);
}

uint32_t ToxAV::toxavAudioIterationInterval(void) const {
	return toxav_audio_iteration_interval(_tox_av);
}

void ToxAV::toxavAudioIterate(void) {
	toxav_audio_iterate(_tox_av);
}

uint32_t ToxAV::toxavVideoIterationInterval(void) const {
	return toxav_video_iteration_interval(_tox_av);
}

void ToxAV::toxavVideoIterate(void) {
	toxav_video_iterate(_tox_av);
}

Toxav_Err_Call ToxAV::toxavCall(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate) {
	Toxav_Err_Call err {TOXAV_ERR_CALL_OK};
	toxav_call(_tox_av, friend_number, audio_bit_rate, video_bit_rate, &err);
	return err;
}

Toxav_Err_Answer ToxAV::toxavAnswer(uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate) {
	Toxav_Err_Answer err {TOXAV_ERR_ANSWER_OK};
	toxav_answer(_tox_av, friend_number, audio_bit_rate, video_bit_rate, &err);
	return err;
}

Toxav_Err_Call_Control ToxAV::toxavCallControl(uint32_t friend_number, Toxav_Call_Control control) {
	Toxav_Err_Call_Control err {TOXAV_ERR_CALL_CONTROL_OK};
	toxav_call_control(_tox_av, friend_number, control, &err);
	return err;
}

Toxav_Err_Send_Frame ToxAV::toxavAudioSendFrame(uint32_t friend_number, const int16_t pcm[], size_t sample_count, uint8_t channels, uint32_t sampling_rate) {
	Toxav_Err_Send_Frame err {TOXAV_ERR_SEND_FRAME_OK};
	toxav_audio_send_frame(_tox_av, friend_number, pcm, sample_count, channels, sampling_rate, &err);
	return err;
}

Toxav_Err_Bit_Rate_Set ToxAV::toxavAudioSetBitRate(uint32_t friend_number, uint32_t bit_rate) {
	Toxav_Err_Bit_Rate_Set err {TOXAV_ERR_BIT_RATE_SET_OK};
	toxav_audio_set_bit_rate(_tox_av, friend_number, bit_rate, &err);
	return err;
}

Toxav_Err_Send_Frame ToxAV::toxavVideoSendFrame(uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t y[], const uint8_t u[], const uint8_t v[]) {
	Toxav_Err_Send_Frame err {TOXAV_ERR_SEND_FRAME_OK};
	toxav_video_send_frame(_tox_av, friend_number, width, height, y, u, v, &err);
	return err;
}

Toxav_Err_Bit_Rate_Set ToxAV::toxavVideoSetBitRate(uint32_t friend_number, uint32_t bit_rate) {
	Toxav_Err_Bit_Rate_Set err {TOXAV_ERR_BIT_RATE_SET_OK};
	toxav_video_set_bit_rate(_tox_av, friend_number, bit_rate, &err);
	return err;
}

