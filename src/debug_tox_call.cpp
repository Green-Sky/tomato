#include "./debug_tox_call.hpp"

#include "./stream_manager.hpp"
#include "./content/sdl_video_frame_stream2.hpp"

#include <SDL3/SDL.h>

#include <imgui/imgui.h>

#include <cstring>
#include <cstdint>

#include <iostream>
#include <memory>

// fwd
namespace Message {
	uint64_t getTimeMS();
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

static bool isFormatPlanar(SDL_PixelFormat f) {
	return
		f == SDL_PIXELFORMAT_YV12 ||
		f == SDL_PIXELFORMAT_IYUV ||
		f == SDL_PIXELFORMAT_YUY2 ||
		f == SDL_PIXELFORMAT_UYVY ||
		f == SDL_PIXELFORMAT_YVYU ||
		f == SDL_PIXELFORMAT_NV12 ||
		f == SDL_PIXELFORMAT_NV21 ||
		f == SDL_PIXELFORMAT_P010
	;
}

static SDL_Surface* convertYUY2_IYUV(SDL_Surface* surf) {
	if (surf->format != SDL_PIXELFORMAT_YUY2) {
		return nullptr;
	}
	if ((surf->w % 2) != 0) {
		SDL_SetError("YUY2->IYUV does not support odd widths");
		// hmmm, we dont handle odd widths
		return nullptr;
	}

	SDL_LockSurface(surf);

	SDL_Surface* conv_surf = SDL_CreateSurface(surf->w, surf->h, SDL_PIXELFORMAT_IYUV);
	SDL_LockSurface(conv_surf);

	// YUY2 is 4:2:2 packed
	// Y is simple, we just copy it over
	// U V are double the resolution (vertically), so we avg both
	// Packed mode: Y0+U0+Y1+V0 (1 plane)

	uint8_t* y_plane = static_cast<uint8_t*>(conv_surf->pixels);
	uint8_t* u_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h;
	uint8_t* v_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h + (conv_surf->w/2)*(conv_surf->h/2);

	const uint8_t* yuy2_data = static_cast<const uint8_t*>(surf->pixels);

	for (int y = 0; y < surf->h; y++) {
		for (int x = 0; x < surf->w; x += 2) {
			// every pixel uses 2 bytes
			const uint8_t* yuy2_curser = yuy2_data + y*surf->w*2 + x*2;
			uint8_t src_y0 = yuy2_curser[0];
			uint8_t src_u = yuy2_curser[1];
			uint8_t src_y1 = yuy2_curser[2];
			uint8_t src_v = yuy2_curser[3];

			y_plane[y*conv_surf->w + x] = src_y0;
			y_plane[y*conv_surf->w + x+1] = src_y1;

			size_t uv_index = (y/2) * (conv_surf->w/2) + x/2;
			if (y % 2 == 0) {
				// first write
				u_plane[uv_index] = src_u;
				v_plane[uv_index] = src_v;
			} else {
				// second write, mix with existing value
				u_plane[uv_index] = (int(u_plane[uv_index]) + int(src_u)) / 2;
				v_plane[uv_index] = (int(v_plane[uv_index]) + int(src_v)) / 2;
			}
		}
	}

	SDL_UnlockSurface(conv_surf);
	SDL_UnlockSurface(surf);
	return conv_surf;
}

struct PushConversionQueuedVideoStream : public QueuedFrameStream2<SDLVideoFrame> {
	SDL_PixelFormat _forced_format {SDL_PIXELFORMAT_IYUV};

	PushConversionQueuedVideoStream(size_t queue_size, bool lossy = true) : QueuedFrameStream2<SDLVideoFrame>(queue_size, lossy) {}
	~PushConversionQueuedVideoStream(void) {}

	bool push(const SDLVideoFrame& value) override {
		SDL_Surface* converted_surf = value.surface.get();
		if (converted_surf->format != _forced_format) {
			//std::cerr << "DTC: need to convert from " << SDL_GetPixelFormatName(converted_surf->format) << " to SDL_PIXELFORMAT_IYUV\n";
			if (converted_surf->format == SDL_PIXELFORMAT_YUY2 && _forced_format == SDL_PIXELFORMAT_IYUV) {
				// optimized custom impl

				//auto start = Message::getTimeMS();
				converted_surf = convertYUY2_IYUV(converted_surf);
				//auto end = Message::getTimeMS();
				// 3ms
				//std::cerr << "DTC: timing " << SDL_GetPixelFormatName(converted_surf->format) << "->SDL_PIXELFORMAT_IYUV: " << end-start << "ms\n";
			} else if (isFormatPlanar(converted_surf->format)) {
				// meh, need to convert to rgb as a stopgap

				//auto start = Message::getTimeMS();
				//SDL_Surface* tmp_conv_surf = SDL_ConvertSurfaceAndColorspace(converted_surf, SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
				SDL_Surface* tmp_conv_surf = SDL_ConvertSurfaceAndColorspace(converted_surf, SDL_PIXELFORMAT_RGB24, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
				//auto end = Message::getTimeMS();
				// 1ms
				//std::cerr << "DTC: timing " << SDL_GetPixelFormatName(converted_surf->format) << "->SDL_PIXELFORMAT_RGB24: " << end-start << "ms\n";

				// TODO: fix sdl rgb->yuv conversion resulting in too dark (colorspace) issues
				//start = Message::getTimeMS();
				converted_surf = SDL_ConvertSurfaceAndColorspace(tmp_conv_surf, SDL_PIXELFORMAT_IYUV, nullptr, SDL_COLORSPACE_YUV_DEFAULT, 0);
				//end = Message::getTimeMS();
				// 60ms
				//std::cerr << "DTC: timing SDL_PIXELFORMAT_RGB24->SDL_PIXELFORMAT_IYUV: " << end-start << "ms\n";

				SDL_DestroySurface(tmp_conv_surf);
			} else {
				converted_surf = SDL_ConvertSurface(converted_surf, SDL_PIXELFORMAT_IYUV);
			}

			if (converted_surf == nullptr) {
				// oh god
				std::cerr << "DTC error: failed to convert surface to IYUV: " << SDL_GetError() << "\n";
				return false;
			}
		}
		assert(converted_surf != nullptr);
		if (converted_surf != value.surface.get()) {
			// TODO: add ctr with uptr
			SDLVideoFrame new_value{value.timestampUS, nullptr};
			new_value.surface = {
				converted_surf,
				&SDL_DestroySurface
			};

			return QueuedFrameStream2<SDLVideoFrame>::push(std::move(new_value));
		} else {
			return QueuedFrameStream2<SDLVideoFrame>::push(value);
		}
	}
};

// exlusive
// TODO: replace with something better than a queue
struct ToxAVCallVideoSink : public FrameStream2SinkI<SDLVideoFrame> {
	uint32_t _fid;
	std::shared_ptr<PushConversionQueuedVideoStream> _writer;

	ToxAVCallVideoSink(uint32_t fid) : _fid(fid) {}
	~ToxAVCallVideoSink(void) {}

	// sink
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		if (_writer) {
			// max 1 (exclusive)
			return nullptr;
		}

		// TODO: enable video here
		_writer = std::make_shared<PushConversionQueuedVideoStream>(10, true);

		return _writer;
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		if (!sub || !_writer) {
			// nah
			return false;
		}

		if (sub == _writer) {
			// TODO: disable video here
			_writer = nullptr;
			return true;
		}

		// what
		return false;
	}
};

DebugToxCall::DebugToxCall(ObjectStore2& os, ToxAV& toxav, TextureUploaderI& tu) : _os(os), _toxav(toxav), _tu(tu) {
	_toxav.subscribe(this, ToxAV_Event::friend_call);
	_toxav.subscribe(this, ToxAV_Event::friend_call_state);
	_toxav.subscribe(this, ToxAV_Event::friend_audio_bitrate);
	_toxav.subscribe(this, ToxAV_Event::friend_video_bitrate);
	_toxav.subscribe(this, ToxAV_Event::friend_audio_frame);
	_toxav.subscribe(this, ToxAV_Event::friend_video_frame);
}

void DebugToxCall::tick(float) {
	// pump sink to tox
	// TODO: own thread or direct on push
	// TODO: pump at double the frame rate
	for (const auto& [oc, vsink] : _os.registry().view<ToxAVCallVideoSink*>().each()) {
		if (!vsink->_writer) {
			continue;
		}

		auto new_frame_opt = vsink->_writer->pop();
		if (!new_frame_opt.has_value()) {
			continue;
		}

		if (!new_frame_opt.value().surface) {
			// wtf?
			continue;
		}

		// conversion is done in the sinks stream
		SDL_Surface* surf = new_frame_opt.value().surface.get();
		assert(surf != nullptr);

		SDL_LockSurface(surf);
		_toxav.toxavVideoSendFrame(
			vsink->_fid,
			surf->w, surf->h,
			static_cast<const uint8_t*>(surf->pixels),
			static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h,
			static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h + (surf->w/2) * (surf->h/2)
		);
		SDL_UnlockSurface(surf);
	}
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
					//const auto ret = _toxav.toxavAnswer(fid, 0, 1); // 1mbit/s
					const auto ret = _toxav.toxavAnswer(fid, 0, 2); // 2mbit/s
					//const auto ret = _toxav.toxavAnswer(fid, 0, 100); // 100mbit/s
					//const auto ret = _toxav.toxavAnswer(fid, 0, 2500); // 2500mbit/s
					if (ret == TOXAV_ERR_ANSWER_OK) {
						call.incoming = false;

						// create sinks
						call.outgoing_vsink = {_os.registry(), _os.registry().create()};
						{
							auto new_vsink = std::make_unique<ToxAVCallVideoSink>(fid);
							call.outgoing_vsink.emplace<ToxAVCallVideoSink*>(new_vsink.get());
							call.outgoing_vsink.emplace<Components::FrameStream2Sink<SDLVideoFrame>>(std::move(new_vsink));
							call.outgoing_vsink.emplace<Components::StreamSink>("ToxAV friend call video", std::string{entt::type_name<SDLVideoFrame>::value()});
						}

						// create sources
						if (call.incoming_v) {
							call.incoming_vsrc = {_os.registry(), _os.registry().create()};
							{
								auto new_vsrc = std::make_unique<SDLVideoFrameStream2MultiSource>();
								call.incoming_vsrc.emplace<SDLVideoFrameStream2MultiSource*>(new_vsrc.get());
								call.incoming_vsrc.emplace<Components::FrameStream2Source<SDLVideoFrame>>(std::move(new_vsrc));
								call.incoming_vsrc.emplace<Components::StreamSource>("ToxAV friend call video", std::string{entt::type_name<SDLVideoFrame>::value()});
							}
						}
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

						// TODO: stream manager disconnectAll()
						if (static_cast<bool>(call.outgoing_vsink)) {
							call.outgoing_vsink.destroy();
						}
						if (static_cast<bool>(call.incoming_vsrc)) {
							call.incoming_vsrc.destroy();
						}
					}
				}

				//if (ImGui::BeginCombo("audio src", "---")) {
				//    ImGui::EndCombo();
				//}
				//if (ImGui::BeginCombo("video src", "---")) {
				//    ImGui::EndCombo();
				//}
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
	call.state = TOXAV_FRIEND_CALL_STATE_NONE;

	return true;
}

bool DebugToxCall::onEvent(const Events::FriendCallState& e)  {
	auto& call = _calls[e.friend_number];
	call.state = e.state;

	if (
		(call.state & TOXAV_FRIEND_CALL_STATE_FINISHED) != 0 ||
		(call.state & TOXAV_FRIEND_CALL_STATE_ERROR) != 0
	) {
		if (static_cast<bool>(call.outgoing_vsink)) {
			call.outgoing_vsink.destroy();
		}
		if (static_cast<bool>(call.incoming_vsrc)) {
			call.incoming_vsrc.destroy();
		}
	}

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
	// TODO: skip if we dont know about this call
	auto& call = _calls[e.friend_number];

	if (!static_cast<bool>(call.incoming_vsrc)) {
		// missing src to put frame into ??
		return false;
	}

	assert(call.incoming_vsrc.all_of<SDLVideoFrameStream2MultiSource*>());
	assert(call.incoming_vsrc.all_of<Components::FrameStream2Source<SDLVideoFrame>>());

	call.num_v_frames++;

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

	call.incoming_vsrc.get<SDLVideoFrameStream2MultiSource*>()->push({
		// ms -> us
		Message::getTimeMS() * 1000, // TODO: make more precise
		new_surf
	});

	return true;
}

