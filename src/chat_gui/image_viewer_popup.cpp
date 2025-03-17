#include "./image_viewer_popup.hpp"

#include <imgui/imgui.h>

#include <iostream>

ImageViewerPopup::ImageViewerPopup(MessageTextureCache& mtc) : _mtc(mtc) {
}

// open popup with (image) file
void ImageViewerPopup::view(Message3Handle m) {
	if (static_cast<bool>(_m)) {
		std::cout << "IVP warning: overriding open image\n";
	}

	_m = m;

	_open_popup = true;
}

// call this each frame
void ImageViewerPopup::render(float) {
	if (_open_popup) {
		_open_popup = false;
		ImGui::OpenPopup("Image##ImageViewerPopup");
	}

	if (!ImGui::BeginPopup("Image##ImageViewerPopup", ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {
		_m = {}; // meh, event on close would be nice, but the reset is cheap
		_scale = 1.f;
		return;
	}

	ImGui::SliderFloat("scale", &_scale, 0.05f, 2.f);

	auto [id, img_width, img_height] = _mtc.get(_m);

	img_width = std::max<int32_t>(5, _scale * img_width);
	img_height = std::max<int32_t>(5, _scale * img_height);

	ImGui::Image(
		id,
		ImVec2{
			static_cast<float>(img_width),
			static_cast<float>(img_height)
		}
	);

	// TODO: figure out nice scroll zooming
	//ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
	//const auto prev_scale = _scale;
	//_scale += ImGui::GetIO().MouseWheel * 0.05f;
	//_scale = std::clamp(_scale, 0.05f, 3.f);

	//if (std::abs(prev_scale - _scale) > 0.001f) {
	//}

	if (ImGui::Shortcut(ImGuiKey_Escape)) {
		ImGui::CloseCurrentPopup();
		_m = {};
		_scale = 1.f;
	}

	ImGui::EndPopup();
}

