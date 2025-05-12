#include "./qoirdo.hpp"

#define QOI_IMPLEMENTATION
#include "./qoi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ios>
#include <vector>
#include <cstdlib>

void print_help(const char* exe) {
	std::cout << exe << " [-q 1-100] <path_to.qoi>\n";
}

// read qoi image, reencode lossy with rdo
int main(int argc, const char** argv) {
	if (argc < 2) {
		std::cerr << "error: at least one paramenter required.\n";
		std::cout << "help:\n";
		print_help(argv[0]);
		return -1;
	}

	std::filesystem::path input_qoi;

	int quality = 80;

	if (argv[1] == std::string_view{"-q"}) {
		if (argc < 4) {
			std::cerr << "error: more parameters required\n";
			std::cout << "help:\n";
			print_help(argv[0]);
			return -1;
		}

		quality = std::atoi(argv[2]);
		if (quality < 1 || quality > 100) {
			std::cerr << "error: invalid quality\n";
			std::cout << "help:\n";
			print_help(argv[0]);
			return -1;
		}

		input_qoi = argv[3];
	} else {
		input_qoi = argv[1];
	}

	std::filesystem::path output_qoi;
	if (input_qoi.extension() == ".qoi" || input_qoi.extension() == ".QOI") {
		output_qoi = input_qoi;
		output_qoi.replace_extension("rdo.qoi");
	} else {
		output_qoi = input_qoi;
		output_qoi.replace_filename(input_qoi.filename().generic_u8string() + std::string{".rdo.qoi"});
	}

	std::cout << "input_qoi: " << input_qoi.generic_u8string() << "\n";
	std::cout << "output_qoi: " << output_qoi.generic_u8string() << "\n";
	std::cout << "quality: " << quality << "\n";

	std::vector<uint8_t> input_encoded_data;
	size_t input_file_size {0};
	{ // read file
		std::ifstream ifile{input_qoi, std::ios::in | std::ios::binary};
		if (!ifile.is_open()) {
			std::cerr << "failed to open file " << input_qoi << "\n";
			return -2;
		}
		ifile.seekg(0, std::ios_base::end);
		const auto size = ifile.tellg();
		if (size <= 0) {
			std::cerr << "failed to open file " << input_qoi << ", file too small\n";
			return -2;
		}
		ifile.seekg(0, std::ios_base::beg);
		input_encoded_data.resize(size);
		ifile.read(reinterpret_cast<char*>(input_encoded_data.data()), input_encoded_data.size());
		input_file_size = size;
	}

	// decode

	qoi_desc input_desc{};
	uint8_t* raw_image = static_cast<uint8_t*>(qoi_decode(input_encoded_data.data(), input_encoded_data.size(), &input_desc, 4));

	if (raw_image == nullptr) {
		std::cerr << "failed to decode input\n";
		return -3;
	}
	if (input_desc.width == 0 || input_desc.height == 0) {
		free(raw_image);
		std::cerr << "funny trying to decode input\n";
		return -3;
	}

	init_qoi_rdo();

	// encode with rdo

	qoi_rdo_desc desc{
		input_desc.width,
		input_desc.height,
		/*input_desc.channels*/ 4, // ?
		input_desc.colorspace,
	};
	std::vector<uint8_t> encoded_data = encode_qoi_rdo_simple(raw_image, desc,quality);
	free(raw_image);

	quit_qoi_rdo();

	if (encoded_data.empty()) {
		std::cerr << "failed to encode image\n";
		return -3;
	}
	if (encoded_data.size() < 4) {
		std::cout << "warn: encoded image suspiciously small\n";
	}

	{ // write out
		std::ofstream ofile{output_qoi, std::ios::out | std::ios::binary | std::ios::trunc};

		if (!ofile.is_open()) {
			std::cerr << "failed to open output file " << output_qoi << "\n";
			return -2;
		}

		ofile.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
	}

	std::cout << "written " << encoded_data.size() << " bytes to " << output_qoi << ". input was " << input_file_size << "\n";

	// TODO: metrics

	return 0;
}

