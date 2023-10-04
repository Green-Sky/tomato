#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

struct SendImagePopup {
	TextureUploaderI& _tu;

	// private
	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	// copy of the original data, dont touch!
	std::vector<uint8_t> original_data;
	bool original_raw {false};
	std::string original_file_ext; // if !original_raw

	ImageLoaderI::ImageResult original_image;

	struct Rect {
		uint32_t x {0};
		uint32_t y {0};

		uint32_t w {0};
		uint32_t h {0};
	};
	Rect crop_rect;

	// texture to render (orig img)
	TextureEntry preview_image;

	bool _open_popup {false};

	std::function<void(const std::vector<uint8_t>&, std::string_view)> _on_send = [](const auto&, auto){};
	std::function<void(void)> _on_cancel = [](){};

	void reset(void);

	// loads the image in original_data
	// fills in original_image, preview_image and crop_rect
	// returns if loaded successfully
	bool load(void);

	public:
		SendImagePopup(TextureUploaderI& tu);

		void sendMemory(
			const uint8_t* data, size_t data_size,
			std::function<void(const std::vector<uint8_t>&, std::string_view)>&& on_send,
			std::function<void(void)>&& on_cancel
		);
		// from memory_raw
		// from file_path

		// call this each frame
		void render(void);
};

