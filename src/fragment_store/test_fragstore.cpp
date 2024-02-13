#include <cstdint>
#include <iostream>

#include "./fragment_store.hpp"

#include <nlohmann/json.hpp>
#include <entt/entity/handle.hpp>

namespace Components {
	struct MessagesTimestampRange {
		uint64_t begin {0};
		uint64_t end {1000};
	};
} // Components


static bool serl_json_msg_ts_range(void* comp, nlohmann::json& out) {
	if (comp == nullptr) {
		return false;
	}

	out = nlohmann::json::object();

	auto& r_comp = *reinterpret_cast<Components::MessagesTimestampRange*>(comp);

	out["begin"] = r_comp.begin;
	out["end"] = r_comp.end;

	return true;
}

int main(void) {
	FragmentStore fs;
	fs._default_store_path = "test_store/";
	fs._sc.registerSerializerJson<Components::MessagesTimestampRange>(serl_json_msg_ts_range);

	const auto frag0 = fs.newFragmentFile("", MetaFileType::TEXT_JSON, {0xff, 0xf1, 0xf2, 0xf0, 0xff, 0xff, 0xff, 0xf9});

	const auto frag1 = fs.newFragmentFile("", MetaFileType::BINARY_MSGPACK);

	const auto frag2 = fs.newFragmentFile("", MetaFileType::BINARY_MSGPACK);

	{
		auto frag0h = fs.fragmentHandle(frag0);

		frag0h.emplace_or_replace<Components::DataCompressionType>();
		frag0h.emplace_or_replace<Components::DataEncryptionType>();
		frag0h.emplace_or_replace<Components::MessagesTimestampRange>();

		std::function<FragmentStore::write_to_storage_fetch_data_cb> fn_cb = [read = 0ul](uint8_t* request_buffer, uint64_t buffer_size) mutable -> uint64_t {
			uint64_t i = 0;
			for (; i+read < 3000 && i < buffer_size; i++) {
				request_buffer[i] = uint8_t((i+read) & 0xff);
			}
			read += i;

			return i;
		};
		fs.syncToStorage(frag0, fn_cb);
	}

	{
		auto frag1h = fs.fragmentHandle(frag1);

		frag1h.emplace_or_replace<Components::DataCompressionType>();
		frag1h.emplace_or_replace<Components::DataEncryptionType>();

		std::function<FragmentStore::write_to_storage_fetch_data_cb> fn_cb = [read = 0ul](uint8_t* request_buffer, uint64_t buffer_size) mutable -> uint64_t {
			static constexpr std::string_view text = "This is some random data";
			uint64_t i = 0;
			for (; i+read < text.size() && i < buffer_size; i++) {
				request_buffer[i] = text[i+read];
			}
			read += i;

			return i;
		};
		fs.syncToStorage(frag1, fn_cb);
	}

	return 0;
}

