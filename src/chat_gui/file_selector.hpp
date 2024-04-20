#pragma once

#include <filesystem>
#include <functional>

struct FileSelector {
	std::filesystem::path _current_file_path = std::filesystem::canonical(".") / ""; // add /

	bool _open_popup {false};

	std::function<bool(std::filesystem::path& path)> _is_valid = [](auto){ return true; };
	std::function<void(const std::filesystem::path& path)> _on_choose = [](auto){};
	std::function<void(void)> _on_cancel = [](){};

	void reset(void);

	public:
		FileSelector(void);

		// TODO: supply hints
		// HACK: until we supply hints, is_valid can modify
		void requestFile(
			std::function<bool(std::filesystem::path& path)>&& is_valid,
			std::function<void(const std::filesystem::path& path)>&& on_choose,
			std::function<void(void)>&& on_cancel
		);

		// call this each frame
		void render(void);
};

