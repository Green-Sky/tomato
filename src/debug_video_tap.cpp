#include "./debug_video_tap.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <entt/entity/entity.hpp>

#include <SDL3/SDL.h>

#include <imgui/imgui.h>

#include "./frame_streams/sdl/video.hpp"
#include "./frame_streams/frame_stream2.hpp"
#include "./frame_streams/locked_frame_stream.hpp"
#include "./frame_streams/sdl/video_push_converter.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <deque>

#include <thread>
#include <chrono>
#include <atomic>

#include <iostream>

// fwd
namespace Message {
uint64_t getTimeMS(void);
}

struct DebugVideoTapSink : public FrameStream2SinkI<SDLVideoFrame> {
	TextureUploaderI& _tu;

	uint32_t _id_counter {0};

	struct Writer {
		struct View {
			uint32_t _id {0}; // for stable imgui ids

			uint64_t _tex {0};
			uint32_t _tex_w {0};
			uint32_t _tex_h {0};

			bool _mirror {false}; // flip horizontally

			uint64_t _v_last_ts {0}; // us
			float _v_interval_avg {0.f}; // s
		} view;

		std::shared_ptr<PushConversionVideoStream<LockedFrameStream2<SDLVideoFrame>>> stream;
	};
	std::vector<Writer> _writers;

	DebugVideoTapSink(TextureUploaderI& tu) : _tu(tu) {}
	~DebugVideoTapSink(void) {}

	// sink
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		_writers.emplace_back(Writer{
			Writer::View{_id_counter++},
			std::make_shared<PushConversionVideoStream<LockedFrameStream2<SDLVideoFrame>>>(SDL_PIXELFORMAT_IYUV)
		});

		return _writers.back().stream;
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		if (!sub || _writers.empty()) {
			// nah
			return false;
		}

		for (auto it = _writers.cbegin(); it != _writers.cend(); it++) {
			if (it->stream == sub) {
				_tu.destroy(it->view._tex);
				_writers.erase(it);
				return true;
			}
		}

		// what
		return false;
	}
};

struct DebugVideoTestSource : public FrameStream2SourceI<SDLVideoFrame> {
	std::vector<std::shared_ptr<LockedFrameStream2<SDLVideoFrame>>> _readers;

	std::atomic_bool _stop {false};
	std::thread _thread;

	DebugVideoTestSource(void) {
		std::cout << "DVTS: starting new test video source\n";
		_thread = std::thread([this](void) {
			while (!_stop) {
				if (!_readers.empty()) {
					auto* surf = SDL_CreateSurface(960, 720, SDL_PIXELFORMAT_RGBA32);

					// color
					static auto start_time = Message::getTimeMS();
					const float time = (Message::getTimeMS() - start_time)/1000.f;
					SDL_ClearSurface(surf, std::sin(time), std::cos(time), 0.5f, 1.f);

					SDLVideoFrame frame{ // non-owning
						Message::getTimeMS()*1000,
						surf,
					};

					for (auto& stream : _readers) {
						stream->push(frame); // copy
					}

					SDL_DestroySurface(surf);
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		});
	}
	~DebugVideoTestSource(void) {
		_stop = true;
		_thread.join();
	}

	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		return _readers.emplace_back(std::make_shared<LockedFrameStream2<SDLVideoFrame>>());
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		for (auto it = _readers.cbegin(); it != _readers.cend(); it++) {
			if (it->get() == sub.get()) {
				_readers.erase(it);
				return true;
			}
		}

		return false;
	}
};

DebugVideoTap::DebugVideoTap(ObjectStore2& os, StreamManager& sm, TextureUploaderI& tu) : _os(os), _sm(sm), _tu(tu) {
	// post self as video sink
	_tap = {_os.registry(), _os.registry().create()};
	try {
		auto dvts = std::make_unique<DebugVideoTapSink>(_tu);
		_tap.emplace<DebugVideoTapSink*>(dvts.get()); // to get our data back
		_tap.emplace<Components::FrameStream2Sink<SDLVideoFrame>>(
			std::move(dvts)
		);

		_tap.emplace<Components::StreamSink>(Components::StreamSink::create<SDLVideoFrame>("DebugVideoTap"));
		_tap.emplace<Components::TagDefaultTarget>();

		_os.throwEventConstruct(_tap);
	} catch (...) {
		_os.registry().destroy(_tap);
	}

	_src = {_os.registry(), _os.registry().create()};
	try {
		auto dvts = std::make_unique<DebugVideoTestSource>();
		_src.emplace<DebugVideoTestSource*>(dvts.get());
		_src.emplace<Components::FrameStream2Source<SDLVideoFrame>>(
			std::move(dvts)
		);

		_src.emplace<Components::StreamSource>(Components::StreamSource::create<SDLVideoFrame>("DebugVideoTest"));

		_os.throwEventConstruct(_src);
	} catch (...) {
		_os.registry().destroy(_src);
	}
}

DebugVideoTap::~DebugVideoTap(void) {
	if (static_cast<bool>(_tap)) {
		_os.registry().destroy(_tap);
	}
	if (static_cast<bool>(_src)) {
		_os.registry().destroy(_src);
	}
}

float DebugVideoTap::render(void) {
	float min_interval {2.f};
	auto& dvtsw = _tap.get<DebugVideoTapSink*>()->_writers;
	for (auto& [view, stream] : dvtsw) {
		std::string window_title {"DebugVideoTap #"};
		window_title += std::to_string(view._id);
		ImGui::SetNextWindowSize({400, 420}, ImGuiCond_Appearing);
		if (ImGui::Begin(window_title.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar)) {
			while (auto new_frame_opt = stream->pop()) {
				// timing
				if (view._v_last_ts == 0) {
					view._v_last_ts = new_frame_opt.value().timestampUS;
				} else {
					auto delta = int64_t(new_frame_opt.value().timestampUS) - int64_t(view._v_last_ts);
					view._v_last_ts = new_frame_opt.value().timestampUS;

					if (view._v_interval_avg == 0) {
						view._v_interval_avg = delta/1'000'000.f;
					} else {
						const float r = 0.05f;
						view._v_interval_avg = view._v_interval_avg * (1.f-r) + (delta/1'000'000.f) * r;
					}
				}

				SDL_Surface* new_frame_surf = new_frame_opt.value().surface.get();

				SDL_Surface* converted_surf = new_frame_surf;
				//if (new_frame_surf->format != SDL_PIXELFORMAT_RGBA32) {
				//    // we need to convert
				//    //std::cerr << "DVT: need to convert\n";
				//    converted_surf = SDL_ConvertSurfaceAndColorspace(new_frame_surf, SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
				//    assert(converted_surf->format == SDL_PIXELFORMAT_RGBA32);
				//}

				SDL_LockSurface(converted_surf);
				if (view._tex == 0 || (int)view._tex_w != converted_surf->w || (int)view._tex_h != converted_surf->h) {
					_tu.destroy(view._tex);
					view._tex = _tu.upload(
						static_cast<const uint8_t*>(converted_surf->pixels),
						converted_surf->w,
						converted_surf->h,
						TextureUploaderI::IYUV, // forced conversion
						TextureUploaderI::LINEAR,
						TextureUploaderI::STREAMING
					);

					view._tex_w = converted_surf->w;
					view._tex_h = converted_surf->h;
				} else {
					//_tu.update(view._tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 4);
					_tu.update(view._tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 3/2);
					//_tu.updateRGBA(view._tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 4);
				}
				SDL_UnlockSurface(converted_surf);

				//if (new_frame_surf != converted_surf) {
				//    // clean up temp
				//    SDL_DestroySurface(converted_surf);
				//}
			}

			ImGui::Checkbox("mirror ", &view._mirror);

			// img here
			if (view._tex != 0) {
				ImGui::SameLine();
				ImGui::Text("%dx%d interval: ~%.0fms (%.2ffps)", view._tex_w, view._tex_h, view._v_interval_avg*1000.f, 1.f/view._v_interval_avg);
				const float img_w = ImGui::GetContentRegionAvail().x;
				ImGui::Image(
					reinterpret_cast<ImTextureID>(view._tex),
					ImVec2{img_w, img_w * float(view._tex_h)/view._tex_w},
					ImVec2{view._mirror?1.f:0.f, 0},
					ImVec2{view._mirror?0.f:1.f, 1}
				);
			}

			if (view._v_interval_avg > 0) {
				min_interval = std::min(min_interval, view._v_interval_avg);
			}
		}
		ImGui::End();
	}

	return min_interval;
}

