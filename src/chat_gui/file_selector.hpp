#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <future>

struct FileSelector {
	std::filesystem::path _current_file_path;

	struct CachedData {
		std::filesystem::path file_path; // can be used to check against current
		std::vector<std::filesystem::directory_entry> dirs;
		std::vector<std::filesystem::directory_entry> files;
	};
	std::optional<CachedData> _current_cache;
	std::future<CachedData> _future_cache;

	bool _open_popup {false};

	std::function<bool(std::filesystem::path& path)> _is_valid = [](auto){ return true; };
	std::function<void(const std::filesystem::path& path)> _on_choose = [](auto){};
	std::function<void(void)> _on_cancel = [](){};

	void reset(void);

	public:
		FileSelector(void);
		~FileSelector(void);

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

