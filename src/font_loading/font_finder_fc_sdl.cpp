#include "./font_finder_fc_sdl.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <string_view>
#include <algorithm>

#include <iostream>

bool FontFinder_FontConfigSDL::fillCache(void) {
	const char *args[] = {"fc-list", "-f", "%{family}:%{style}:%{lang}:%{color}\n", NULL};
	auto* proc = SDL_CreateProcess(args, true);
	if (proc == nullptr) {
		throw std::runtime_error("failed to create process");
		return false;
	}

	size_t data_size{0};
	int exit_code{0};
	char* data = static_cast<char*>(SDL_ReadProcess(proc, &data_size, &exit_code));
	if (data == nullptr) {
		throw std::runtime_error("process returned no data");
		return false; // error
	}
	if (exit_code != 0) {
		SDL_free(data);
		throw std::runtime_error("process exit code " + std::to_string(exit_code));
		return false;
	}

	std::string_view returned_string{data, data_size};

	//std::cout << "fc-list:\n" << returned_string << "\n";

	static constexpr auto for_each_split = [](std::string_view input, const char sep, auto&& fn) {
		for (auto next = input.find_first_of(sep); next != std::string_view::npos; next = input.find_first_of(sep)) {
			auto segment = input.substr(0, next);
			input = input.substr(next+1);
			fn(segment);
		}
		if (!input.empty()) {
			fn(input);
		}
	};

	// cached outside
	std::vector<std::string_view> segment_splits;
	// split by newline
	for_each_split(returned_string, '\n', [&segment_splits, this](auto line) {
		if (line.size() == 0) {
			return;
		}

		segment_splits.clear(); // preserves cap

		// then split by colon (TODO:back to front)
		for_each_split(line, ':', [&segment_splits](auto segment) {
			segment_splits.push_back(segment);
		});

		//"%{family}:%{style}:%{lang}:%{color}\n"
		if (segment_splits.size() < 4) {
			std::cerr << "invalid font line! (s:" << segment_splits.size() << ")\n";
			return;
		}

		if (segment_splits[0].empty()) {
			// empty family, warn?
			return;
		}

		std::string processed_family{segment_splits[0]};
		processed_family.erase(std::remove(processed_family.begin(), processed_family.end(), ' '), processed_family.end());
		std::transform(processed_family.begin(), processed_family.end(), processed_family.begin(), [](unsigned char c) { return std::tolower(c); });

		std::string processed_lang{segment_splits[2]};
		std::transform(processed_lang.begin(), processed_lang.end(), processed_lang.begin(), [](unsigned char c) { return std::tolower(c); });

		_cache.push_back(SystemFont{
			std::string{segment_splits[0]},
			processed_family,
			processed_lang,
			segment_splits[3] == "True" || segment_splits[3] == "true" || segment_splits[3] == "1",
		});
#if 0
		segment_splits[0]; // family
		segment_splits[1]; // style
		segment_splits[2]; // lang
		// langs are concatinated with |
		segment_splits[3]; // color
#endif
	});

	SDL_free(data);
	return true;
}

std::string FontFinder_FontConfigSDL::getFontFile(const std::string& font_name) {
	const char *args[] = {"fc-match", "-f", "%{file}", font_name.c_str(), NULL};

	auto* proc = SDL_CreateProcess(args, true);
	if (proc == nullptr) {
		std::cerr << "failed to create process\n";
		return "";
	}

	size_t data_size{0};
	int exit_code{0};
	char* data = static_cast<char*>(SDL_ReadProcess(proc, &data_size, &exit_code));
	if (data == nullptr) {
		std::cerr << "process returned no data\n";
		return "";
	}
	if (exit_code != 0) {
		SDL_free(data);
		std::cerr << "process exit code " << exit_code << "\n";
		return "";
	}

	std::string_view returned_string{data, data_size};

	//std::cout << "fc-match -f file:\n" << returned_string << "\n";

	std::string return_string{returned_string};

	SDL_free(data);

	return return_string;
}

FontFinder_FontConfigSDL::FontFinder_FontConfigSDL(void) {
	fillCache();
}

void FontFinder_FontConfigSDL::reset(void) {
	_cache.clear();
	fillCache();
}

const char* FontFinder_FontConfigSDL::name(void) const {
	return "FontConfigSDL";
}

// TODO: use fc-match instead, maybe fallback to this
std::string FontFinder_FontConfigSDL::findBest(std::string_view family, std::string_view lang, bool color) const {
	const SystemFont* best_ptr = nullptr;
	int best_score = 0;

	if (_cache.empty()) {
		std::cerr << "Empty system cache\n";
	}

	for (const auto& it : _cache) {
		int score = 0;
		if (it.family == family) {
			score += 10;
		} else {
			std::string processed_family = std::string{family};
			processed_family.erase(std::remove(processed_family.begin(), processed_family.end(), ' '), processed_family.end());
			std::transform(processed_family.begin(), processed_family.end(), processed_family.begin(), [](unsigned char c) { return std::tolower(c); });

			if (it.processed_family == processed_family) {
				score += 9;
			} else {
				// search for family as substring in it.family (processed)
				if (it.processed_family.find(processed_family) != std::string::npos) {
					score += 2;
				}
			}
		}
		// TODO: case/fuzzy compare/edit distance

		if (score > 0) { // extra selectors after the family
			if (lang.empty()) {
				if (!it.lang.empty()) {
					score -= 1;
				}
			} else {
				if (it.lang.find(lang) != std::string::npos) {
					score += 5;
				}
			}

			// TODO: style? or make style part of the name?

			if (it.color != color) {
				score -= 1;
			}
		}

		if (best_score < score) {
			best_score = score;
			best_ptr = &it;
		}

		//if (score > 0 && lang == "ja") {
		//    std::cout << it.family << " l:" << it.lang << " had score " << score << "\n";
		//}
	}

	if (best_ptr == nullptr) {
		return "";
	} else {
		return getFontFile(best_ptr->family);
	}
}

// TODO: remove
void fc_sdl_get_font_emoji(void) {
	const char *args[] = {"fc-match", "-s", "-f", "%{file}\n%{color}\n", "emoji", NULL};
	// results in eg:
	// /nix/store/i8r5whmw46b2p3a4n5ls878jjnlpz4mq-noto-fonts-color-emoji-2.047/share/fonts/noto/NotoColorEmoji.ttf/True
	// /usr/share/fonts/noto/NotoEmoji.ttf/False
	// ...

	auto* proc = SDL_CreateProcess(args, true);
	if (proc == nullptr) {
		std::cerr << "failed to create process\n";
		return; // error
	}

	size_t data_size{0};
	int exit_code{0};
	char* data = static_cast<char*>(SDL_ReadProcess(proc, &data_size, &exit_code));
	if (data == nullptr) {
		std::cerr << "process returned no data\n";
		return; // error
	}
	if (exit_code != 0) {
		std::cerr << "process exit code " << exit_code << "\n";
		return; // error
	}

	std::string_view returned_string{data, data_size};

	std::cout << "fc-match emoji:\n" << returned_string << "\n";

	SDL_free(data);
}
