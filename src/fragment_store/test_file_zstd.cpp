#include "./file2_zstd.hpp"

#include <solanaceae/util/span.hpp>
#include <solanaceae/file/file2_std.hpp>

#include <filesystem>
#include <iostream>
#include <variant>
#include <algorithm>
#include <vector>
#include <random>
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
	const auto temp_dir = std::filesystem::temp_directory_path() / "file2_zstd_tests";

	std::filesystem::create_directories(temp_dir); // making sure
	assert(std::filesystem::exists(temp_dir));
	std::cout << "test temp dir: " << temp_dir << "\n";

	const auto test1_file_path = temp_dir / "testfile1.zstd";
	{ // simple write test
		File2WFile f_w_file{std::string_view{test1_file_path.u8string()}, true};
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
		File2RFile f_r_file{std::string_view{test1_file_path.u8string()}};
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

			assert(r_res_vec.size() == data_test_text1.size);
			assert(std::equal(data_test_text1.cbegin(), data_test_text1.cend(), r_res_vec.cbegin()));
		}

		{ // readback data_test_text2
			auto r_res_var = f_r_zstd.read(data_test_text2.size);

			//assert(f_r_zstd.isGood());
			//assert(f_r_file.isGood());
			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);

			//std::cout << "decomp: " << std::string_view{reinterpret_cast<const char*>(r_res_vec.data()), r_res_vec.size()};

			assert(r_res_vec.size() == data_test_text2.size);
			assert(std::equal(
				data_test_text2.cbegin(),
				data_test_text2.cend(),
				r_res_vec.cbegin()
			));
		}

		{ // readback data_test_text3
			auto r_res_var = f_r_zstd.read(data_test_text3.size);

			//assert(f_r_zstd.isGood());
			//assert(f_r_file.isGood());
			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);

			//std::cout << "decomp: " << std::string_view{reinterpret_cast<const char*>(r_res_vec.data()), r_res_vec.size()};

			assert(r_res_vec.size() == data_test_text3.size);
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

	const auto test2_file_path = temp_dir / "testfile2.zstd";
	{ // write and read a single frame with increasing size
		for (size_t fslog = 1; fslog <= 25; fslog++) {
			const size_t frame_size = 1<<fslog;
			//std::cerr << "fs: " << frame_size << "\n";

			{ // write
				std::minstd_rand rng_data{11*1337};

				File2WFile f_w_file{std::string_view{test2_file_path.u8string()}, true};
				assert(f_w_file.isGood());

				File2ZSTDW f_w_zstd{f_w_file};
				assert(f_w_zstd.isGood());
				assert(f_w_file.isGood());

				std::vector<uint8_t> tmp_data(frame_size);
				for (auto& e : tmp_data) {
					e = uint8_t(rng_data() & 0xff); // cutoff bad but good enough
				}
				assert(tmp_data.size() == frame_size);

				bool res = f_w_zstd.write(ByteSpan{tmp_data});
				assert(res);
				assert(f_w_zstd.isGood());
				assert(f_w_file.isGood());
			}

			{ // read
				std::minstd_rand rng_data{11*1337};

				File2RFile f_r_file{std::string_view{test2_file_path.u8string()}};
				assert(f_r_file.isGood());

				File2ZSTDR f_r_zstd{f_r_file};
				assert(f_r_zstd.isGood());
				assert(f_r_file.isGood());

				{ // read frame
					auto r_res_var = f_r_zstd.read(frame_size);

					assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
					const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);
					assert(r_res_vec.size() == frame_size);

					// assert equal
					for (auto& e : r_res_vec) {
						assert(e == uint8_t(rng_data() & 0xff));
					}
				}

				{ // eof test
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

			// since we spam file, we immediatly remove them
			std::filesystem::remove(test2_file_path);
		}
	}

	const auto test3_file_path = temp_dir / "testfile3.zstd";
	{ // large file test write
		File2WFile f_w_file{std::string_view{test3_file_path.u8string()}, true};
		assert(f_w_file.isGood());

		File2ZSTDW f_w_zstd{f_w_file};
		assert(f_w_zstd.isGood());
		assert(f_w_file.isGood());

		std::minstd_rand rng{11*1337};
		std::minstd_rand rng_data{11*1337}; // make investigating easier

		size_t total_raw_size {0};
		for (size_t i = 0; i < 2000; i++) {
			const size_t frame_size = (rng() % ((2<<19) - 1)) + 1;

			std::vector<uint8_t> tmp_data(frame_size);
			for (auto& e : tmp_data) {
				e = uint8_t(rng_data() & 0xff); // cutoff bad but good enough
			}

			bool res = f_w_zstd.write(ByteSpan{tmp_data});
			assert(res);
			assert(f_w_zstd.isGood());
			assert(f_w_file.isGood());
			total_raw_size += frame_size;
		}
		std::cout << "t3 total raw size: " << total_raw_size << "\n";
	}

	// after flush
	std::cout << "t3 size on disk:   " << std::filesystem::file_size(test3_file_path) << "\n";

	{ // large file test read
		File2RFile f_r_file{std::string_view{test3_file_path.u8string()}};
		assert(f_r_file.isGood());

		File2ZSTDR f_r_zstd{f_r_file};
		assert(f_r_zstd.isGood());
		assert(f_r_file.isGood());

		// using same rng state as write to compare
		std::minstd_rand rng{11*1337};
		std::minstd_rand rng_data{11*1337};

		for (size_t i = 0; i < 2000; i++) {
			const size_t frame_size = (rng() % ((2<<19) - 1)) + 1;
			//std::cerr << "f: " << i << " fs: " << frame_size << "\n";

			auto r_res_var = f_r_zstd.read(frame_size);

			assert(std::holds_alternative<std::vector<uint8_t>>(r_res_var));
			const auto& r_res_vec = std::get<std::vector<uint8_t>>(r_res_var);
			assert(r_res_vec.size() == frame_size);

			// assert equal
			for (auto& e : r_res_vec) {
				assert(e == uint8_t(rng_data() & 0xff));
			}
		}

		{ // eof test
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

