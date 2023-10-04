#include "./send_image_popup.hpp"

#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_stb.hpp"
#include "./image_loader_webp.hpp"

#include <imgui/imgui.h>

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

		//const auto TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
		//const auto TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		//ImGui::SetNextWindowSize({TEXT_BASE_WIDTH*100, TEXT_BASE_HEIGHT*30});
	}

	// TODO: add cancel action
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
					+ ImGui::GetFrameHeightWithSpacing()
				)
			;
			if (height > max_height) {
				width *= max_height / height;
				height = max_height;
			}

			// TODO: propergate type
			ImGui::Image(
				preview_image.getID<void*>(),
				ImVec2{static_cast<float>(width), static_cast<float>(height)}
			);
		}

		if (ImGui::Button("X cancel", {ImGui::GetWindowContentRegionWidth()/2.f, TEXT_BASE_HEIGHT*2})) {
			_on_cancel();
			ImGui::CloseCurrentPopup();
			reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("send ->", {-FLT_MIN, TEXT_BASE_HEIGHT*2})) {
			//if (_is_valid(_current_file_path)) {

				// if modified
				//_on_send(data);
				// else
				_on_send(original_data, original_file_ext);

				ImGui::CloseCurrentPopup();
				reset();
			//}
		}

		ImGui::EndPopup();
	}
}

