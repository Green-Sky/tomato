#include "./send_image_popup.hpp"

#include "../image_loader_sdl_bmp.hpp"
#include "../image_loader_stb.hpp"
#include "../image_loader_webp.hpp"
#include "../image_loader_qoi.hpp"
#include "../image_loader_sdl_image.hpp"

#include <solanaceae/util/time.hpp>

#include <filesystem>
#include <solanaceae/file/file2_std.hpp>

#include <imgui.h>

#include <cmath>

SendImagePopup::SendImagePopup(TextureUploaderI& tu) : _tu(tu) {
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderQOI>());
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLImage>());
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

	cropping = false;
	dragging_last_frame_ul = false;
	dragging_last_frame_lr = false;
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
		crop_rect.x = original_image.width * 0.05f;
		crop_rect.y = original_image.height * 0.05f;
		crop_rect.w = original_image.width * 0.9f;
		crop_rect.h = original_image.height * 0.9f;
#endif

		crop_rect = sanitizeCrop(crop_rect, original_image.width, original_image.height);
		crop_before_drag = crop_rect;

		original_file_ext = ".";
		if (original_image.file_ext != nullptr) {
			original_file_ext += original_image.file_ext;
		} else {
			// HACK: manually probe for png
			if (!original_raw
				&& original_data.size() >= 4
				&& original_data.at(0) == 0x89
				&& original_data.at(1) == 'P'
				&& original_data.at(2) == 'N'
				&& original_data.at(3) == 'G'
			) {
				original_file_ext += "png";
			} else {
				original_file_ext += "unk"; // very meh, default to png?
			}
		}

		assert(preview_image.textures.empty());
		preview_image.timestamp_last_rendered = getTimeMS();
		preview_image.current_texture = 0;
		for (const auto& [ms, data] : original_image.frames) {
			const auto n_t = _tu.upload(data.data(), original_image.width, original_image.height);
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

SendImagePopup::Rect SendImagePopup::sanitizeCrop(Rect crop_rect, int32_t image_width, int32_t image_height) {
	// w and h min is 1 -> x/y need to be smaller so the img is atleast 1px in any dim
	if (crop_rect.x >= image_width-1) {
		crop_rect.x = image_width-2;
	} else if (crop_rect.x < 0) {
		crop_rect.x = 0;
	}

	if (crop_rect.y >= image_height-1) {
		crop_rect.y = image_height-2;
	} else if (crop_rect.y < 0) {
		crop_rect.y = 0;
	}

	if (crop_rect.w > image_width - crop_rect.x) {
		crop_rect.w = image_width - crop_rect.x;
	} else if (crop_rect.w < 1) {
		crop_rect.w = 1;
		// TODO: adjust X
	}

	if (crop_rect.h > image_height - crop_rect.y) {
		crop_rect.h = image_height - crop_rect.y;
	} else if (crop_rect.h < 1) {
		crop_rect.h = 1;
		// TODO: adjust Y
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
	original_data = {data, data+data_size};

	if (!load()) {
		std::cerr << "SIP: failed to load image from memory\n";
		reset();
		return;
	}

	_open_popup = true;

	_on_send = std::move(on_send);
	_on_cancel = std::move(on_cancel);
}

bool SendImagePopup::sendFilePath( // file2 instead?
	std::string_view file_path,
	std::function<void(const std::vector<uint8_t>&, std::string_view)>&& on_send,
	std::function<void(void)>&& on_cancel
) {
	original_raw = false;
	if (file_path.empty() || !std::filesystem::exists(file_path)) {
		return false; // error
	}

	std::filesystem::path path_o{file_path};

	const auto file_size = std::filesystem::file_size(path_o);
	if (file_size <= 0 || file_size > 100*1024*1024) { // limit to 100mib for now
		return false; // error
	}

	File2RFile file2{file_path};
	if (!file2.isGood() || !file2.can_read) {
		std::cerr << "filed to read file '" << file_path << "'\n";
	}

	{ // copy paste data to memory
		// inefficent
		const auto data = file2.read(file_size, 0);
		original_data = {data.ptr, data.ptr+data.size};
	}

	if (path_o.has_extension()) {
		original_file_ext = path_o.extension().u8string();
	}

	if (!load()) {
		std::cerr << "SIP: failed to load image from file '" << file_path << "'\n";
		reset();
		return false;
	}

	_open_popup = true;

	_on_send = std::move(on_send);
	_on_cancel = std::move(on_cancel);
	return true;
}

void SendImagePopup::render(float time_delta) {
	if (_open_popup) {
		_open_popup = false;
		ImGui::OpenPopup("send image##SendImagePopup");
	}

	// TODO: add cancel shortcut (esc)
	if (!ImGui::BeginPopupModal("send image##SendImagePopup", nullptr/*, ImGuiWindowFlags_NoDecoration*/)) {
		return;
	}

	//const auto TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
	const auto TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	preview_image.doAnimation(getTimeMS());

	time += time_delta;
	time = fmod(time, 1.f); // fract()

	//ImGui::Text("send file....\n......");

	{
		float width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		float height = crop_rect.h * (width / crop_rect.w);
		if (cropping) {
			height = original_image.height * (width / original_image.width);
		}

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

		// TODO: propergate texture id type

		// save curser pos
		const auto pre_img_curser_screen = ImGui::GetCursorScreenPos();
		const auto pre_img_curser = ImGui::GetCursorPos();

		if (cropping) { // if cropping
			// display full image
			ImGui::Image(
				preview_image.getID<ImTextureID>(),
				ImVec2{static_cast<float>(width), static_cast<float>(height)}
			);
			const auto post_img_curser = ImGui::GetCursorPos();


			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.5f, 0.5f, 0.5f, 0.2f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.5f, 0.5f, 0.5f, 0.4f});

			auto ul_clipper_pos = ImVec2{float(crop_rect.x)/original_image.width, float(crop_rect.y)/original_image.height};
			{ // crop upper left clipper

				ImGui::SetCursorPos({
					pre_img_curser.x + ul_clipper_pos.x * width,
					pre_img_curser.y + ul_clipper_pos.y * height
				});

				ImGui::Button("##ul_clipper", {TEXT_BASE_HEIGHT, TEXT_BASE_HEIGHT});
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					if (dragging_last_frame_ul) {
						auto drag_total = ImGui::GetMouseDragDelta();
						drag_total.x = (drag_total.x / width) * original_image.width;
						drag_total.y = (drag_total.y / height) * original_image.height;

						crop_rect.x = std::max<float>(crop_before_drag.x + drag_total.x, 0.01f);
						crop_rect.y = std::max<float>(crop_before_drag.y + drag_total.y, 0.01f);

						crop_rect.x = std::min<int32_t>(crop_rect.x, original_image.width-2);
						crop_rect.y = std::min<int32_t>(crop_rect.y, original_image.height-2);

						crop_rect.w = crop_before_drag.w - (crop_rect.x - crop_before_drag.x);
						crop_rect.h = crop_before_drag.h - (crop_rect.y - crop_before_drag.y);
					} else {
						if (ImGui::IsItemActive()) {
							dragging_last_frame_ul = true;
							// drag started on button, start drag
						}
					}
				} else if (dragging_last_frame_ul) { // was dragging
					dragging_last_frame_ul = false;
					crop_before_drag = crop_rect;
				}
			}

			auto lr_clipper_pos = ImVec2{float(crop_rect.x+crop_rect.w)/original_image.width, float(crop_rect.y+crop_rect.h)/original_image.height};
			{ // crop lower right clipper
				ImGui::SetCursorPos({
					pre_img_curser.x + lr_clipper_pos.x * width - TEXT_BASE_HEIGHT,
					pre_img_curser.y + lr_clipper_pos.y * height - TEXT_BASE_HEIGHT
				});

				ImGui::Button("##lr_clipper", {TEXT_BASE_HEIGHT, TEXT_BASE_HEIGHT});
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					if (dragging_last_frame_lr) {
						auto drag_total = ImGui::GetMouseDragDelta();
						drag_total.x = (drag_total.x / width) * original_image.width;
						drag_total.y = (drag_total.y / height) * original_image.height;

						crop_rect.w = std::min<float>(crop_before_drag.w + drag_total.x, original_image.width);
						crop_rect.h = std::min<float>(crop_before_drag.h + drag_total.y, original_image.height);
					} else {
						if (ImGui::IsItemActive()) {
							dragging_last_frame_lr = true;
							// drag started on button, start drag
						}
					}
				} else if (dragging_last_frame_lr) { // was dragging
					dragging_last_frame_lr = false;
					crop_before_drag = crop_rect;
				}
			}

			// sanitzie after tool
			crop_rect = sanitizeCrop(crop_rect, original_image.width, original_image.height);

			{ // 4 lines delimiting the crop result
				ImU32 line_color = 0xffffffff;
				{ // calc color
					auto rgb = [](float x) -> ImVec4 {
						auto f = [](float x) {
							while (x < 0.f) {
								x += 1.f;
							}

							x = std::fmod(x, 1.f); // fract()

							if (x < 1.f/3) {
								return x * 3;
							} else if (x < 2.f/3) {
								return (1 - (x - (1.f/3))) * 3 - 2;
							} else {
								return 0.f;
							}
						};

						float red = f(x);
						float green = f(x - (1.f/3));
						float blue = f(x - (2.f/3));

						return {red, green, blue, 1.f};
					};

					line_color = ImGui::GetColorU32(rgb(time));
				}

				// x vertical
				ImGui::GetWindowDrawList()->AddLine(
					{pre_img_curser_screen.x + ul_clipper_pos.x * width, pre_img_curser_screen.y},
					{pre_img_curser_screen.x + ul_clipper_pos.x * width, pre_img_curser_screen.y + height},
					line_color,
					1.f
				);

				// y horizontal
				ImGui::GetWindowDrawList()->AddLine(
					{pre_img_curser_screen.x,			pre_img_curser_screen.y + ul_clipper_pos.y * height},
					{pre_img_curser_screen.x + width,	pre_img_curser_screen.y + ul_clipper_pos.y * height},
					line_color,
					1.f
				);

				// w vertical
				ImGui::GetWindowDrawList()->AddLine(
					{pre_img_curser_screen.x + lr_clipper_pos.x * width, pre_img_curser_screen.y},
					{pre_img_curser_screen.x + lr_clipper_pos.x * width, pre_img_curser_screen.y + height},
					line_color,
					1.f
				);

				// h horizontal
				ImGui::GetWindowDrawList()->AddLine(
					{pre_img_curser_screen.x,			pre_img_curser_screen.y + lr_clipper_pos.y * height},
					{pre_img_curser_screen.x + width,	pre_img_curser_screen.y + lr_clipper_pos.y * height},
					line_color,
					1.f
				);
			}

			// cancel/ok buttons in the img center?

			ImGui::PopStyleColor(2);

			ImGui::SetCursorPos(post_img_curser);
		} else {
			crop_rect = sanitizeCrop(crop_rect, original_image.width, original_image.height);

			// display cropped area
			ImGui::Image(
				preview_image.getID<ImTextureID>(),
				ImVec2{static_cast<float>(width), static_cast<float>(height)},
				ImVec2{float(crop_rect.x)/original_image.width, float(crop_rect.y)/original_image.height},
				ImVec2{float(crop_rect.x+crop_rect.w)/original_image.width, float(crop_rect.y+crop_rect.h)/original_image.height}
			);

			// transparent crop button on image
#if 0
			const auto post_img_curser = ImGui::GetCursorPos();

			ImGui::SetCursorPos(pre_img_curser);

			// TODO: fancy cropping toggle
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.5f, 0.5f, 0.5f, 0.2f});
			if (ImGui::Button("crop", {TEXT_BASE_WIDTH*8, TEXT_BASE_HEIGHT*2})) {
				cropping = true;
			}
			ImGui::PopStyleColor();

			ImGui::SetCursorPos(post_img_curser);
#endif
		}
	}

	const bool cropped = crop_rect.x != 0 || crop_rect.y != 0 || crop_rect.w != original_image.width || crop_rect.h != original_image.height;
	if (cropping) {
		if (ImGui::Button("done")) {
			cropping = false;
		}
	} else {
		if (ImGui::Button("crop")) {
			cropping = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("reset")) {
		crop_rect.x = 0;
		crop_rect.y = 0;
		crop_rect.w = original_image.width;
		crop_rect.h = original_image.height;
		crop_before_drag = crop_rect;
	}
	ImGui::SameLine();
	ImGui::Text("x:%d y:%d w:%d h:%d", crop_rect.x, crop_rect.y, crop_rect.w, crop_rect.h);

	bool recalc_size = false;
	if (cropped) {
		if (!compress) {
			// looks like a change
			recalc_size = true;
		}
		compress = true;
	}

	recalc_size |= ImGui::Checkbox("compress", &compress);
	if (cropped && ImGui::IsItemHovered()) {
		ImGui::SetTooltip("required since cropped!");
	}

	static int current_compressor = 0;

	if (compress) {
		ImGui::SameLine();
		ImGui::Combo("##compression_type", &current_compressor, "webp\0webp lossless\0jpeg\0png\0qoi\0qoi lossy\0");

		ImGui::Indent();
		// combo "webp""webp-lossless""png""jpg?"
		// if lossy quality slider (1-100) default 80
		if (current_compressor == 0 || current_compressor == 2 || current_compressor == 5) {
			const uint32_t qmin = 1;
			const uint32_t qmax = 100;
			recalc_size |= ImGui::SliderScalar("quality", ImGuiDataType_U32, &quality, &qmin, &qmax);
		}
		if (current_compressor == 1 || current_compressor == 3) {
			const uint32_t qmin = 0;
			const uint32_t qmax = 9;
			recalc_size |= ImGui::SliderScalar("compression_level", ImGuiDataType_U32, &compression_level, &qmin, &qmax);
		}

		if (recalc_size) {
			// compress and save temp? cooldown? async? save size only?
			// print size where?
		}

		ImGui::Unindent();
	}

	//if (ImGui::Button("X cancel", {ImGui::GetWindowContentRegionWidth()/2.f, TEXT_BASE_HEIGHT*2})) {
	if (ImGui::Button("X cancel", {ImGui::GetContentRegionAvail().x/2.f, TEXT_BASE_HEIGHT*2})) {
		_on_cancel();
		ImGui::CloseCurrentPopup();
		reset();
	}
	ImGui::SameLine();
	if (ImGui::Button("send ->", {-FLT_MIN, TEXT_BASE_HEIGHT*2})) {
		if (compress || cropped) {
			// TODO: copy bad
			ImageLoaderI::ImageResult tmp_img;
			if (cropped) {
				std::cout << "SIP: CROP!!!!!\n";
				tmp_img = original_image.crop(
					crop_rect.x,
					crop_rect.y,
					crop_rect.w,
					crop_rect.h
				);
			} else {
				tmp_img = original_image;
			}

			std::vector<uint8_t> new_data;

			// HACK: generic list
			if (current_compressor == 0) {
				new_data = ImageEncoderWebP{}.encodeToMemoryRGBA(tmp_img, {{"quality", quality}});
				if (!new_data.empty()) {
					_on_send(new_data, ".webp");
				}
			} else if (current_compressor == 1) {
				new_data = ImageEncoderWebP{}.encodeToMemoryRGBA(tmp_img, {{"compression_level", compression_level}});
				if (!new_data.empty()) {
					_on_send(new_data, ".webp");
				}
			} else if (current_compressor == 2) {
				new_data = ImageEncoderSTBJpeg{}.encodeToMemoryRGBA(tmp_img, {{"quality", quality}});
				if (!new_data.empty()) {
					_on_send(new_data, ".jpg");
				}
			} else if (current_compressor == 3) {
				new_data = ImageEncoderSTBPNG{}.encodeToMemoryRGBA(tmp_img, {{"png_compression_level", compression_level}});
				if (!new_data.empty()) {
					_on_send(new_data, ".png");
				}
			} else if (current_compressor == 4) {
				new_data = ImageEncoderQOI{}.encodeToMemoryRGBA(tmp_img, {});
				if (!new_data.empty()) {
					_on_send(new_data, ".qoi");
				}
			} else if (current_compressor == 5) {
				new_data = ImageEncoderQOI{}.encodeToMemoryRGBA(tmp_img, {{"quality", quality}});
				if (!new_data.empty()) {
					_on_send(new_data, ".qoi");
				}
			}
			// error

		} else {
			_on_send(original_data, original_file_ext);
		}

		ImGui::CloseCurrentPopup();
		reset();
	}

	if (ImGui::Shortcut(ImGuiKey_Escape)) {
		ImGui::CloseCurrentPopup();
		reset();
	}

	ImGui::EndPopup();
}

