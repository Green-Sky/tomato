#include "./send_image_popup.hpp"

#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_stb.hpp"
#include "./image_loader_webp.hpp"

#include <imgui/imgui.h>

// tmp
#include <webp/mux.h>
#include <webp/encode.h>

SendImagePopup::SendImagePopup(TextureUploaderI& tu) : _tu(tu) {
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSTB>());
}

void SendImagePopup::reset(void) {
	_on_send = [](const auto&, auto){};
	_on_cancel = [](){};

	// hm
	original_data.clear();
	{
		original_image.width = 0;
		original_image.height = 0;
		original_image.frames.clear();
	}

	// clear preview img
	for (const auto& tex_id : preview_image.textures) {
		_tu.destroy(tex_id);
	}
	preview_image = {};
}

bool SendImagePopup::load(void) {
	// try all loaders after another
	for (auto& il : _image_loaders) {
		original_image = il->loadFromMemoryRGBA(original_data.data(), original_data.size());
		if (original_image.frames.empty() || original_image.height == 0 || original_image.width == 0) {
			continue;
		}

#if 1
		crop_rect.x = 0;
		crop_rect.y = 0;
		crop_rect.w = original_image.width;
		crop_rect.h = original_image.height;
#else
		crop_rect.x = original_image.width * 0.1f;
		crop_rect.y = original_image.height * 0.1f;
		crop_rect.w = original_image.width * 0.8f;
		crop_rect.h = original_image.height * 0.8f;

#endif
		crop_rect = sanitizeCrop(crop_rect, original_image.width, original_image.height);

		original_file_ext = ".";
		if (original_image.file_ext != nullptr) {
			original_file_ext += original_image.file_ext;
		} else {
			original_file_ext += "unk";
		}

		preview_image.timestamp_last_rendered = getNowMS();
		preview_image.current_texture = 0;
		for (const auto& [ms, data] : original_image.frames) {
			const auto n_t = _tu.uploadRGBA(data.data(), original_image.width, original_image.height);
			preview_image.textures.push_back(n_t);
			preview_image.frame_duration.push_back(ms);
		}

		// redundant
		preview_image.width = original_image.width;
		preview_image.height = original_image.height;

		if (original_image.frames.size() > 1) {
			std::cout << "SIP: loaded animation\n";
		} else {
			std::cout << "SIP: loaded image\n";
		}

		return true;
	}

	return false;
}

std::vector<uint8_t> SendImagePopup::compressWebp(const ImageLoaderI::ImageResult& input_image, uint32_t quality) {
	// HACK: move to own interface

	WebPAnimEncoderOptions enc_options;
	if (!WebPAnimEncoderOptionsInit(&enc_options)) {
		std::cerr << "SIP error: WebPAnimEncoderOptionsInit()\n";
		return {};
	}

	// Tune 'enc_options' as needed.
	enc_options.minimize_size = 1; // might be slow? optimize for size, no key-frame insertion

	WebPAnimEncoder* enc = WebPAnimEncoderNew(input_image.width, input_image.height, &enc_options);
	if (enc == nullptr) {
		std::cerr << "SIP error: WebPAnimEncoderNew()\n";
		return {};
	}

	int prev_timestamp = 0;
	for (const auto& frame : input_image.frames) {
		WebPConfig config;
		//WebPConfigInit(&config);
		if (!WebPConfigPreset(&config, WebPPreset::WEBP_PRESET_DEFAULT, quality)) {
			std::cerr << "SIP error: WebPConfigPreset()\n";
			WebPAnimEncoderDelete(enc);
			return {};
		}
		//WebPConfigLosslessPreset(&config, 6); // 9 for max compression

		WebPPicture frame_webp;
		if (!WebPPictureInit(&frame_webp)) {
			std::cerr << "SIP error: WebPPictureInit()\n";
			WebPAnimEncoderDelete(enc);
			return {};
		}
		frame_webp.width = input_image.width;
		frame_webp.height = input_image.height;
		if (!WebPPictureImportRGBA(&frame_webp, frame.data.data(), 4*input_image.width)) {
			std::cerr << "SIP error: WebPPictureImportRGBA()\n";
			WebPAnimEncoderDelete(enc);
			return {};
		}

		if (!WebPAnimEncoderAdd(enc, &frame_webp, prev_timestamp, &config)) {
			std::cerr << "SIP error: WebPAnimEncoderAdd()\n";
			WebPPictureFree(&frame_webp);
			WebPAnimEncoderDelete(enc);
			return {};
		}
		prev_timestamp += frame.ms;
	}
	if (!WebPAnimEncoderAdd(enc, NULL, prev_timestamp, NULL)) { // tell anim encoder its the end
		std::cerr << "SIP error: WebPAnimEncoderAdd(NULL)\n";
		WebPAnimEncoderDelete(enc);
		return {};
	}

	WebPData webp_data;
	WebPDataInit(&webp_data);
	if (!WebPAnimEncoderAssemble(enc, &webp_data)) {
		std::cerr << "SIP error: WebPAnimEncoderAdd(NULL)\n";
		WebPAnimEncoderDelete(enc);
		return {};
	}
	WebPAnimEncoderDelete(enc);

	// Write the 'webp_data' to a file, or re-mux it further.
	// TODO: make it not a copy
	std::vector<uint8_t> new_data{webp_data.bytes, webp_data.bytes+webp_data.size};

	WebPDataClear(&webp_data);

	return new_data;
}

ImageLoaderI::ImageResult SendImagePopup::crop(const ImageLoaderI::ImageResult& input_image, const Rect& crop_rect) {
	// TODO: proper error handling
	assert(crop_rect.x+crop_rect.w <= input_image.width);
	assert(crop_rect.y+crop_rect.h <= input_image.height);

	ImageLoaderI::ImageResult new_image;
	new_image.width = crop_rect.w;
	new_image.height = crop_rect.h;
	new_image.file_ext = input_image.file_ext;

	for (const auto& input_frame : input_image.frames) {
		auto& new_frame = new_image.frames.emplace_back();
		new_frame.ms = input_frame.ms;

		// TODO: improve this, this is super inefficent
		for (int64_t y = crop_rect.y; y < crop_rect.y + crop_rect.h; y++) {
			for (int64_t x = crop_rect.x; x < crop_rect.x + crop_rect.w; x++) {
				new_frame.data.push_back(input_frame.data.at(y*input_image.width*4+x*4+0));
				new_frame.data.push_back(input_frame.data.at(y*input_image.width*4+x*4+1));
				new_frame.data.push_back(input_frame.data.at(y*input_image.width*4+x*4+2));
				new_frame.data.push_back(input_frame.data.at(y*input_image.width*4+x*4+3));
			}
		}
	}

	return new_image;
}

SendImagePopup::Rect SendImagePopup::sanitizeCrop(Rect crop_rect, uint32_t image_width, uint32_t image_height) {
	// w and h min is 1 -> x/y need to be smaller so the img is atleast 1px in any dim
	if (crop_rect.x >= image_width-1) {
		crop_rect.x = image_width-2;
	}
	if (crop_rect.y >= image_height-1) {
		crop_rect.y = image_height-2;
	}

	if (crop_rect.w > crop_rect.x + image_width) {
		crop_rect.w = crop_rect.x + image_width;
	}
	if (crop_rect.h > crop_rect.y + image_height) {
		crop_rect.h = crop_rect.y + image_height;
	}

	return crop_rect;
}

void SendImagePopup::sendMemory(
	const uint8_t* data, size_t data_size,
	std::function<void(const std::vector<uint8_t>&, std::string_view)>&& on_send,
	std::function<void(void)>&& on_cancel
) {
	original_raw = false;
	if (data == nullptr || data_size == 0) {
		return; // error
	}

	// copy paste data to memory
	original_data.clear();
	original_data.insert(original_data.begin(), data, data+data_size);

	if (!load()) {
		std::cerr << "SIP: failed to load image from memory\n";
		reset();
		return;
	}

	_open_popup = true;

	_on_send = std::move(on_send);
	_on_cancel = std::move(on_cancel);

}

void SendImagePopup::render(void) {
	if (_open_popup) {
		_open_popup = false;
		ImGui::OpenPopup("send image##SendImagePopup");
	}

	// TODO: add cancel shortcut (esc)
	if (ImGui::BeginPopupModal("send image##SendImagePopup", nullptr/*, ImGuiWindowFlags_NoDecoration*/)) {
		const auto TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		const auto TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		preview_image.doAnimation(getNowMS());

		//ImGui::Text("send file....\n......");

		{
			float width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
			float height = preview_image.height * (width / preview_image.width);

			const float max_height =
				ImGui::GetWindowContentRegionMax().y
				- (
					ImGui::GetWindowContentRegionMin().y
					+ TEXT_BASE_HEIGHT*(2-1) // row of buttons (-1 bc fh inclues fontsize)
					+ ImGui::GetFrameHeightWithSpacing()*4
				)
			;
			if (height > max_height) {
				width *= max_height / height;
				height = max_height;
			}

			// save curser pos
			// TODO: propergate type
			ImGui::Image(
				preview_image.getID<void*>(),
				ImVec2{static_cast<float>(width), static_cast<float>(height)}
			);

			// TODO: crop input here
		}

		crop_rect = sanitizeCrop(crop_rect, original_image.width, original_image.height);
		const bool cropped = crop_rect.x != 0 || crop_rect.y != 0 || crop_rect.w != original_image.width || crop_rect.h != original_image.height;

		bool recalc_size = false;
		if (cropped) {
			if (!compress) {
				// looks like a change
				recalc_size = true;
			}
			compress = true;
		}
		recalc_size |= ImGui::Checkbox("reencode", &compress);
		if (compress) {
			ImGui::Indent();
			// combo "webp""webp-lossless""png""jpg?"
			// if lossy quality slider (1-100) default 80
			const uint32_t qmin = 1;
			const uint32_t qmax = 100;
			recalc_size |= ImGui::SliderScalar("quality", ImGuiDataType_U32, &quality, &qmin, &qmax);

			if (recalc_size) {
				// compress and save temp? cooldown? async? save size only?
				// print size where?
			}

			ImGui::Unindent();
		}

		if (ImGui::Button("X cancel", {ImGui::GetWindowContentRegionWidth()/2.f, TEXT_BASE_HEIGHT*2})) {
			_on_cancel();
			ImGui::CloseCurrentPopup();
			reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("send ->", {-FLT_MIN, TEXT_BASE_HEIGHT*2})) {
			if (compress || cropped) {
				std::vector<uint8_t> new_data;
				if (cropped) {
					std::cout << "SIP: CROP!!!!!\n";
					new_data = compressWebp(crop(original_image, crop_rect), quality);
				} else {
					new_data = compressWebp(original_image, quality);
				}

				if (!new_data.empty()) {
					_on_send(new_data, ".webp");
				}
			} else {
				_on_send(original_data, original_file_ext);
			}

			ImGui::CloseCurrentPopup();
			reset();
		}

		ImGui::EndPopup();
	}
}

