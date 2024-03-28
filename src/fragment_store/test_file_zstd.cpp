#include "./file2_zstd.hpp"

#include <solanaceae/util/span.hpp>
#include <solanaceae/file/file2_std.hpp>

#include <filesystem>
#include <iostream>
#include <variant>
#include <algorithm>
#include <cassert>

const static std::string_view test_text1{"test1 1234 1234 :) 1234 5678 88888888\n"};
const static ByteSpan data_test_text1{
	reinterpret_cast<const uint8_t*>(test_text1.data()),
	test_text1.size()
};

const static std::string_view test_text2{"test2 1234 1234 :) 1234 5678 88888888\n"};
const static ByteSpan data_test_text2{
	reinterpret_cast<const uint8_t*>(test_text2.data()),
	test_text2.size()
};

const static std::string_view test_text3{
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
	"00000000000000000000000000000000000000000000000000000 test 00000000000000000000000000000000000000\n"
};
const static ByteSpan data_test_text3{
	reinterpret_cast<const uint8_t*>(test_text3.data()),
	test_text3.size()
};

int main(void) {
	const auto temp_dir = std::filesystem::temp_directory_path() / "file2wzstdtests";

	std::filesystem::create_directories(temp_dir); // making sure
	assert(std::filesystem::exists(temp_dir));
	std::cout << "test temp dir: " << temp_dir << "\n";

	const auto test1_file_path = temp_dir / "testfile1.zstd";
	{ // simple write test
		File2WFile f_w_file{test1_file_path.c_str(), true};
		assert(f_w_file.isGood());

		File2ZSTDW f_w_zstd{f_w_file};
		assert(f_w_zstd.isGood());
		assert(f_w_file.isGood());

		//bool res = f_w_file.write(data_test_text1);
		bool res = f_w_zstd.write(data_test_text1);
		assert(res);
		assert(f_w_zstd.isGood());
		assert(f_w_file.isGood());

		// write another frame of the same data
		res = f_w_zstd.write(data_test_text2);
		assert(res);
		assert(f_w_zstd.isGood());
		assert(f_w_file.isGood());

		// write larger frame
		res = f_w_zstd.write(data_test_text3);
		assert(res);
		assert(f_w_zstd.isGood());
		assert(f_w_file.isGood());
	}

	// after flush
	assert(std::filesystem::file_size(test1_file_path) != 0);

	{ // simple read test (using write test created file)
		File2RFile f_r_file{test1_file_path.c_str()};
		assert(f_r_file.isGood());

		File2ZSTDR f_r_zstd{f_r_file};
		assert(f_r_zstd.isGood());
		assert(f_r_file.isGood());

		// reads return owning buffers

		{ // readback data_test_text1
			auto r_res_var = f_r_zstd.read(data_test_text1.size);

			//assert(f_r_zstd.isGood());
			//assert(f_r_file.isGood());
			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);

			//std::cout << "decomp: " << std::string_view{reinterpret_cast<const char*>(r_res_vec.data()), r_res_vec.size()};

			assert(std::get<std::vector<uint8_t>>(r_res_var).size() == data_test_text1.size);
			assert(std::equal(data_test_text1.cbegin(), data_test_text1.cend(), std::get<std::vector<uint8_t>>(r_res_var).cbegin()));
		}

		{ // readback data_test_text2
			auto r_res_var = f_r_zstd.read(data_test_text2.size);

			//assert(f_r_zstd.isGood());
			//assert(f_r_file.isGood());
			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);

			//std::cout << "decomp: " << std::string_view{reinterpret_cast<const char*>(r_res_vec.data()), r_res_vec.size()};

			assert(std::get<std::vector<uint8_t>>(r_res_var).size() == data_test_text2.size);
			assert(std::equal(
				data_test_text2.cbegin(),
				data_test_text2.cend(),
				std::get<std::vector<uint8_t>>(r_res_var).cbegin()
			));
		}

		{ // readback data_test_text3
			auto r_res_var = f_r_zstd.read(data_test_text3.size);

			//assert(f_r_zstd.isGood());
			//assert(f_r_file.isGood());
			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);

			//std::cout << "decomp: " << std::string_view{reinterpret_cast<const char*>(r_res_vec.data()), r_res_vec.size()};

			assert(std::get<std::vector<uint8_t>>(r_res_var).size() == data_test_text3.size);
			assert(std::equal(
				data_test_text3.cbegin(),
				data_test_text3.cend(),
				r_res_vec.cbegin()
			));
		}

		{ // assert eof somehow
			// since its eof, reading a single byte should return a zero sized buffer
			auto r_res_var = f_r_zstd.read(1);
			if (std::holds_alternative<std::vector<uint8_t>>(r_res_var)) {
				assert(std::get<std::vector<uint8_t>>(r_res_var).empty());
			} else if (std::holds_alternative<ByteSpan>(r_res_var)) {
				assert(std::get<ByteSpan>(r_res_var).empty());
			} else {
				assert(false);
			}
		}
	}

	// cleanup
	std::filesystem::remove_all(temp_dir);
}

