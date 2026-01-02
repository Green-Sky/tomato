#include "./debug_video_tap.hpp"

#include <solanaceae/util/time.hpp>

#include <solanaceae/object_store/object_store.hpp>

#include <entt/entity/entity.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>

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

		std::shared_ptr<FrameStream2I<SDLVideoFrame>> stream;
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
	std::mutex _readers_mutex;
	std::vector<std::shared_ptr<LockedFrameStream2<SDLVideoFrame>>> _readers;

	std::atomic_bool _stop {false};
	std::thread _thread;

	DebugVideoTestSource(void) {
		std::cout << "DVTS: starting new test video source\n";
		_thread = std::thread([this](void) {
			while (!_stop) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				std::lock_guard lg{_readers_mutex};
				if (!_readers.empty()) {
					auto* surf = SDL_CreateSurface(960, 720, SDL_PIXELFORMAT_RGBA32);

					// color
					static auto start_time = getTimeMS();
					const float time = (getTimeMS() - start_time)/1000.f;
					SDL_ClearSurface(surf, std::sin(time), std::cos(time), 0.5f, 1.f);

					SDLVideoFrame frame{ // non-owning
						getTimeMS()*1000,
						surf,
					};

					for (auto& stream : _readers) {
						stream->push(frame); // copy
					}

					SDL_DestroySurface(surf);
				}
			}
		});
	}
	~DebugVideoTestSource(void) {
		_stop = true;
		_thread.join();
	}

	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		std::lock_guard lg{_readers_mutex};
		return _readers.emplace_back(std::make_shared<LockedFrameStream2<SDLVideoFrame>>());
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		std::lock_guard lg{_readers_mutex};
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
		bool window_open = true;
		if (ImGui::Begin(window_title.c_str(), &window_open, ImGuiWindowFlags_NoScrollbar)) {
			while (auto new_frame_opt = stream->pop()) {
				// timing
				if (view._v_last_ts == 0) {
					view._v_last_ts = new_frame_opt.value().timestampUS;
				} else {
					auto delta = int64_t(new_frame_opt.value().timestampUS) - int64_t(view._v_last_ts);
					view._v_last_ts = new_frame_opt.value().timestampUS;

					if (view._v_interval_avg == 0 || std::isinf(view._v_interval_avg) || std::isnan(view._v_interval_avg)) {
						view._v_interval_avg = delta/1'000'000.f;
					} else {
						const float r = 0.05f;
						view._v_interval_avg = view._v_interval_avg * (1.f-r) + (delta/1'000'000.f) * r;
					}
				}

				SDL_Surface* new_frame_surf = new_frame_opt.value().surface.get();
				SDL_LockSurface(new_frame_surf);
				if (view._tex == 0 || (int)view._tex_w != new_frame_surf->w || (int)view._tex_h != new_frame_surf->h) {
					_tu.destroy(view._tex);
					view._tex = _tu.upload(
						static_cast<const uint8_t*>(new_frame_surf->pixels),
						new_frame_surf->w,
						new_frame_surf->h,
						TextureUploaderI::IYUV, // forced conversion
						TextureUploaderI::LINEAR,
						TextureUploaderI::STREAMING
					);

					view._tex_w = new_frame_surf->w;
					view._tex_h = new_frame_surf->h;
				} else {
					//_tu.update(view._tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 4);
					_tu.update(view._tex, static_cast<const uint8_t*>(new_frame_surf->pixels), new_frame_surf->w * new_frame_surf->h * 3/2);
				}
				SDL_UnlockSurface(new_frame_surf);
			}

			ImGui::Checkbox("mirror ", &view._mirror);

			// img here
			if (view._tex != 0) {
				ImGui::SameLine();
				ImGui::Text("%dx%d interval: ~%.0fms (%.2ffps)", view._tex_w, view._tex_h, view._v_interval_avg*1000.f, 1.f/view._v_interval_avg);
				const float img_w = ImGui::GetContentRegionAvail().x;
				ImGui::Image(
					static_cast<ImTextureID>(static_cast<intptr_t>(view._tex)),
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

		if (!window_open) {
			_sm.forEachConnectedExt(_tap, [this, stream_ptr = stream.get()](ObjectHandle o, const void*, const void* writer) {
				// very hacky
				if (writer == stream_ptr) {
					_sm.disconnect(o, _tap);
				}
			});
		}
	}

	return min_interval;
}

