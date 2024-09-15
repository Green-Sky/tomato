#include "./debug_tox_call.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <imgui/imgui.h>

#include <cstring>

#include <iostream>

// fwd
namespace Message {
	uint64_t getTimeMS(void);
}

static constexpr float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

namespace Components {
	struct ToxAVFriendAudioSource {
	};

	struct ToxAVFriendAudioSink {
	};

	struct ToxAVFriendVideoSource {
	};

	struct ToxAVFriendVideoSink {
	};
}

DebugToxCall::DebugToxCall(ObjectStore2& os, ToxAV& toxav, TextureUploaderI& tu) : _os(os), _toxav(toxav), _tu(tu) {
	_toxav.subscribe(this, ToxAV_Event::friend_call);
	_toxav.subscribe(this, ToxAV_Event::friend_call_state);
	_toxav.subscribe(this, ToxAV_Event::friend_audio_bitrate);
	_toxav.subscribe(this, ToxAV_Event::friend_video_bitrate);
	_toxav.subscribe(this, ToxAV_Event::friend_audio_frame);
	_toxav.subscribe(this, ToxAV_Event::friend_video_frame);
}

void DebugToxCall::tick(float time_delta) {
}

float DebugToxCall::render(void) {
	float next_frame {2.f};
	if (ImGui::Begin("toxav debug")) {
		ImGui::Text("Calls:");
		ImGui::Indent();
		for (auto& [fid, call] : _calls) {
			ImGui::PushID(fid);

			ImGui::Text("fid:%d state:%d", fid, call.state);
			if (call.incoming) {
				ImGui::SameLine();
				if (ImGui::SmallButton("answer")) {
					const auto ret = _toxav.toxavAnswer(fid, 0, 0);
					if (ret == TOXAV_ERR_ANSWER_OK) {
						call.incoming = false;
					}
				}
			} else if (call.state != TOXAV_FRIEND_CALL_STATE_FINISHED) {
				next_frame = std::min(next_frame, 0.1f);
				ImGui::SameLine();
				if (ImGui::SmallButton("hang up")) {
					const auto ret = _toxav.toxavCallControl(fid, TOXAV_CALL_CONTROL_CANCEL);
					if (ret == TOXAV_ERR_CALL_CONTROL_OK) {
						// we hung up
						// not sure if its possible for toxcore to tell this us too when the other side does this at the same time?
						call.state = TOXAV_FRIEND_CALL_STATE_FINISHED;
					}
				}

				//if (ImGui::BeginCombo("audio src", "---")) {
				//    ImGui::EndCombo();
				//}
				//if (ImGui::BeginCombo("video src", "---")) {
				//    ImGui::EndCombo();
				//}
			}

			//if (call.last_v_frame_tex != 0 && ImGui::BeginItemTooltip()) {
			if (call.last_v_frame_tex != 0) {
				next_frame = std::min(next_frame, call.v_frame_interval_avg);
				ImGui::Text("vframe interval avg: %f", call.v_frame_interval_avg);
				ImGui::Image(
					reinterpret_cast<ImTextureID>(call.last_v_frame_tex),
					//ImVec2{float(call.last_v_frame_width), float(call.last_v_frame_height)}
					ImVec2{100.f, 100.f * float(call.last_v_frame_height)/call.last_v_frame_width}
				);
				//ImGui::EndTooltip();
			}

			ImGui::PopID();
		}
		ImGui::Unindent();
	}
	ImGui::End();

	return next_frame;
}

bool DebugToxCall::onEvent(const Events::FriendCall& e)  {
	auto& call = _calls[e.friend_number];
	call.incoming = true;
	call.incoming_a = e.audio_enabled;
	call.incoming_v = e.video_enabled;
	//call.state = TOXAV_FRIEND_CALL_STATE_NONE;

	return true;
}

bool DebugToxCall::onEvent(const Events::FriendCallState& e)  {
	auto& call = _calls[e.friend_number];
	call.state = e.state;

	return true;
}

bool DebugToxCall::onEvent(const Events::FriendAudioBitrate&)  {
	return false;
}

bool DebugToxCall::onEvent(const Events::FriendVideoBitrate&)  {
	return false;
}

bool DebugToxCall::onEvent(const Events::FriendAudioFrame& e)  {
	auto& call = _calls[e.friend_number];
	call.num_a_frames++;
	return false;
}

bool DebugToxCall::onEvent(const Events::FriendVideoFrame& e)  {
	auto& call = _calls[e.friend_number];
	call.num_v_frames++;

	if (call.last_v_frame_timepoint == 0) {
		call.last_v_frame_timepoint = Message::getTimeMS();
	} else {
		const auto new_time_point = Message::getTimeMS();
		auto time_delta_ms = new_time_point - call.last_v_frame_timepoint;
		call.last_v_frame_timepoint = new_time_point;
		time_delta_ms = std::min<uint64_t>(time_delta_ms, 10*1000); // cap at 10sec

		if (call.v_frame_interval_avg == 0) {
			call.v_frame_interval_avg = time_delta_ms/1000.f;
		} else {
			std::cerr << "lerp(" << call.v_frame_interval_avg << ", " << time_delta_ms/1000.f << ", 0.2f) = ";
			call.v_frame_interval_avg = lerp(call.v_frame_interval_avg, time_delta_ms/1000.f, 0.2f);
			std::cerr << call.v_frame_interval_avg << "\n";
		}
	}

	auto* new_surf = SDL_CreateSurface(e.width, e.height, SDL_PIXELFORMAT_IYUV);
	assert(new_surf);
	if (SDL_LockSurface(new_surf)) {
		// copy the data
		// we know how the implementation works, its y u v consecutivlely
		// y
		for (size_t y = 0; y < e.height; y++) {
			std::memcpy(
				//static_cast<uint8_t*>(new_surf->pixels) + new_surf->pitch*y,
				static_cast<uint8_t*>(new_surf->pixels) + e.width*y,
				e.y.ptr + e.ystride*y,
				e.width
			);
		}

		// u
		for (size_t y = 0; y < e.height/2; y++) {
			std::memcpy(
				static_cast<uint8_t*>(new_surf->pixels) + (e.width*e.height) + (e.width/2)*y,
				e.u.ptr + e.ustride*y,
				e.width/2
			);
		}

		// v
		for (size_t y = 0; y < e.height/2; y++) {
			std::memcpy(
				static_cast<uint8_t*>(new_surf->pixels) + (e.width*e.height) + ((e.width/2)*(e.height/2)) + (e.width/2)*y,
				e.v.ptr + e.vstride*y,
				e.width/2
			);
		}

		SDL_UnlockSurface(new_surf);
	}

	auto* converted_surf = SDL_ConvertSurfaceAndColorspace(new_surf, SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_YUV_DEFAULT, 0);
	SDL_DestroySurface(new_surf);
	if (converted_surf == nullptr) {
		assert(false);
		return true;
	}

	SDL_LockSurface(converted_surf);
	if (call.last_v_frame_tex == 0 || call.last_v_frame_width != e.width || call.last_v_frame_height != e.height) {
		_tu.destroy(call.last_v_frame_tex);
		call.last_v_frame_tex = _tu.uploadRGBA(
			static_cast<const uint8_t*>(converted_surf->pixels),
			converted_surf->w,
			converted_surf->h,
			TextureUploaderI::LINEAR,
			TextureUploaderI::STREAMING
		);

		call.last_v_frame_width = e.width;
		call.last_v_frame_height = e.height;
	} else {
		_tu.updateRGBA(call.last_v_frame_tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 4);
	}
	SDL_UnlockSurface(converted_surf);
	SDL_DestroySurface(converted_surf);

	// TODO: use this instead
	//SDL_UpdateYUVTexture(tex, nullptr, e.y.ptr, e.ystride,...

	std::cout << "DTC: updated video texture " << call.last_v_frame_tex << "\n";

	return false;
}

