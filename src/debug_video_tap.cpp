#include "./debug_video_tap.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <entt/entity/entity.hpp>

#include <SDL3/SDL.h>

#include <imgui/imgui.h>

#include "./content/sdl_video_frame_stream2.hpp"

#include <string>
#include <memory>
#include <iostream>

struct DebugVideoTapSink : public FrameStream2SinkI<SDLVideoFrame> {
	std::shared_ptr<QueuedFrameStream2<SDLVideoFrame>> _writer;

	DebugVideoTapSink(void) {}
	~DebugVideoTapSink(void) {}

	// sink
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override {
		if (_writer) {
			// max 1 (exclusive)
			return nullptr;
		}

		_writer = std::make_shared<QueuedFrameStream2<SDLVideoFrame>>(1, true);

		return _writer;
	}

	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override {
		if (!sub || !_writer) {
			// nah
			return false;
		}

		if (sub == _writer) {
			_writer =  nullptr;
			return true;
		}

		// what
		return false;
	}
};

DebugVideoTap::DebugVideoTap(ObjectStore2& os, StreamManager& sm, TextureUploaderI& tu) : _os(os), _sm(sm), _tu(tu) {
	// post self as video sink
	_tap = {_os.registry(), _os.registry().create()};
	try {
		auto dvts = std::make_unique<DebugVideoTapSink>();
		_tap.emplace<DebugVideoTapSink*>(dvts.get()); // to get our data back
		_tap.emplace<Components::FrameStream2Sink<SDLVideoFrame>>(
			std::move(dvts)
		);

		_tap.emplace<Components::StreamSink>("DebugVideoTap", std::string{entt::type_name<SDLVideoFrame>::value()});
	} catch (...) {
		_os.registry().destroy(_tap);
	}
}

DebugVideoTap::~DebugVideoTap(void) {
	if (static_cast<bool>(_tap)) {
		_os.registry().destroy(_tap);
	}
}

float DebugVideoTap::render(void) {
	if (ImGui::Begin("DebugVideoTap")) {
		{ // first pull the latest img from sink and update the texture
			assert(static_cast<bool>(_tap));

			auto& dvtsw = _tap.get<DebugVideoTapSink*>()->_writer;
			if (dvtsw) {
				while (true) {
					auto new_frame_opt = dvtsw->pop();
					if (new_frame_opt.has_value()) {
						// timing
						if (_v_last_ts == 0) {
							_v_last_ts = new_frame_opt.value().timestampUS;
						} else {
							auto delta = int64_t(new_frame_opt.value().timestampUS) - int64_t(_v_last_ts);
							_v_last_ts = new_frame_opt.value().timestampUS;

							//delta = std::min<int64_t>(delta, 10*1000*1000);

							if (_v_interval_avg == 0) {
								_v_interval_avg = delta/1'000'000.f;
							} else {
								const float r = 0.2f;
								_v_interval_avg = _v_interval_avg * (1.f-r) + (delta/1'000'000.f) * r;
							}
						}

						SDL_Surface* new_frame_surf = new_frame_opt.value().surface.get();

						SDL_Surface* converted_surf = new_frame_surf;
						if (new_frame_surf->format != SDL_PIXELFORMAT_RGBA32) {
							// we need to convert
							//std::cerr << "DVT: need to convert\n";
							converted_surf = SDL_ConvertSurfaceAndColorspace(new_frame_surf, SDL_PIXELFORMAT_RGBA32, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
							assert(converted_surf->format == SDL_PIXELFORMAT_RGBA32);
						}

						SDL_LockSurface(converted_surf);
						if (_tex == 0 || (int)_tex_w != converted_surf->w || (int)_tex_h != converted_surf->h) {
							_tu.destroy(_tex);
							_tex = _tu.uploadRGBA(
								static_cast<const uint8_t*>(converted_surf->pixels),
								converted_surf->w,
								converted_surf->h,
								TextureUploaderI::LINEAR,
								TextureUploaderI::STREAMING
							);

							_tex_w = converted_surf->w;
							_tex_h = converted_surf->h;
						} else {
							_tu.updateRGBA(_tex, static_cast<const uint8_t*>(converted_surf->pixels), converted_surf->w * converted_surf->h * 4);
						}
						SDL_UnlockSurface(converted_surf);

						if (new_frame_surf != converted_surf) {
							// clean up temp
							SDL_DestroySurface(converted_surf);
						}
					} else {
						break;
					}
				}
			}
		}

		// list sources dropdown to connect too
		std::string preview_label {"none"};
		if (static_cast<bool>(_selected_src)) {
			preview_label = std::to_string(entt::to_integral(entt::to_entity(_selected_src.entity()))) + " (" + _selected_src.get<Components::StreamSource>().name + ")";
		}

		if (ImGui::BeginCombo("selected source", preview_label.c_str())) {
			if (ImGui::Selectable("none")) {
				switchTo({});
			}

			for (const auto& [oc, ss] : _os.registry().view<Components::StreamSource>().each()) {
				if (ss.frame_type_name != entt::type_name<SDLVideoFrame>::value()) {
					continue;
				}
				std::string label = std::to_string(entt::to_integral(entt::to_entity(oc))) + " (" + ss.name + ")";
				if (ImGui::Selectable(label.c_str())) {
					switchTo({_os.registry(), oc});
				}
			}

			ImGui::EndCombo();
		}

		//ImGui::SetNextItemWidth(0);
		ImGui::Checkbox("mirror", &_mirror);

		// img here
		if (_tex != 0) {
			ImGui::SameLine();
			ImGui::Text("moving avg interval: %f", _v_interval_avg);
			const float img_w = ImGui::GetContentRegionAvail().x;
			ImGui::Image(
				reinterpret_cast<ImTextureID>(_tex),
				ImVec2{img_w, img_w * float(_tex_h)/_tex_w},
				ImVec2{_mirror?1.f:0.f, 0},
				ImVec2{_mirror?0.f:1.f, 1}
			);
		}
	}
	ImGui::End();

	if (_v_interval_avg != 0) {
		return _v_interval_avg;
	} else {
		return 2.f;
	}
}

void DebugVideoTap::switchTo(ObjectHandle o) {
	if (o == _selected_src) {
		std::cerr << "DVT: switch to same ...\n";
		return;
	}

	_tu.destroy(_tex);
	_tex = 0;
	_v_last_ts = 0;
	_v_interval_avg = 0;

	if (static_cast<bool>(_selected_src)) {
		_sm.disconnect<SDLVideoFrame>(_selected_src, _tap);
	}

	if (static_cast<bool>(o) && _sm.connect<SDLVideoFrame>(o, _tap)) {
		_selected_src = o;
	} else {
		std::cerr << "DVT: cleared video source\n";
		_selected_src = {};
	}
}

