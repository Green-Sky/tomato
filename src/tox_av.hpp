#pragma once

#include <tox/toxav.h>

struct ToxAV {
	Tox* _tox = nullptr;
	ToxAV* _tox_av = nullptr;

	ToxAV(Tox* tox);
	virtual ~ToxAV(void);

	// interface
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

};

