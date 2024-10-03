#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/object_store/object_store.hpp>

#include <entt/core/type_info.hpp>

#include "./frame_stream2.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>

// fwd
class StreamManager;

namespace Components {

	// mark a source or sink as the(a) default
	struct TagDefaultTarget {};

	// mark a source/sink as to be connected to a default sink/source
	// of the same type
	struct TagConnectToDefault {};

	struct StreamSource {
		std::string name;
		std::string frame_type_name;

		std::function<bool(StreamManager&, Object, Object, bool)> connect_fn;

		template<typename FrameType>
		static StreamSource create(const std::string& name);
	};

	struct StreamSink {
		std::string name;
		std::string frame_type_name;

		template<typename FrameType>
		static StreamSink create(const std::string& name);
	};

	template<typename FrameType>
	using FrameStream2Source = std::unique_ptr<FrameStream2SourceI<FrameType>>;

	template<typename FrameType>
	using FrameStream2Sink = std::unique_ptr<FrameStream2SinkI<FrameType>>;

} // Components


class StreamManager : protected ObjectStoreEventI {
	friend class StreamManagerUI; // TODO: make this go away
	ObjectStore2& _os;

	struct Connection {
		ObjectHandle src;
		ObjectHandle sink;

		struct Data {
			virtual ~Data(void) {}
		};
		std::unique_ptr<Data> data; // stores reader writer type erased
		std::function<void(Connection&)> pump_fn; // TODO: make it return next interval?
		std::function<void(Connection&)> unsubscribe_fn;

		bool on_main_thread {true};
		std::atomic_bool stop {false}; // disconnect
		std::atomic_bool finished {false}; // disconnect

		// pump thread
		std::thread pump_thread;

		// frame interval counters and estimates
		// TODO

		Connection(void) = default;
		Connection(
			ObjectHandle src_,
			ObjectHandle sink_,
			std::unique_ptr<Data>&& data_,
			std::function<void(Connection&)>&& pump_fn_,
			std::function<void(Connection&)>&& unsubscribe_fn_,
			bool on_main_thread_ = true
		);
	};
	std::vector<std::unique_ptr<Connection>> _connections;

	std::unordered_map<std::string, Object> _default_sources;
	std::unordered_map<std::string, Object> _default_sinks;

	public:
		StreamManager(ObjectStore2& os);
		virtual ~StreamManager(void);

		template<typename FrameType>
		bool connect(Object src, Object sink, bool threaded = true);

		bool connect(Object src, Object sink, bool threaded = true);
		bool disconnect(Object src, Object sink);
		bool disconnectAll(Object o);

		// do we need the time delta?
		float tick(float);

	protected:
		bool onEvent(const ObjectStore::Events::ObjectConstruct&) override;
		bool onEvent(const ObjectStore::Events::ObjectUpdate&) override;
		bool onEvent(const ObjectStore::Events::ObjectDestory&) override;
};

// template impls

namespace Components {

	// we require the complete sm type here
	template<typename FrameType>
	StreamSource StreamSource::create(const std::string& name) {
		return StreamSource{
			name,
			std::string{entt::type_name<FrameType>::value()},
			+[](StreamManager& sm, Object src, Object sink, bool threaded) {
				return sm.connect<FrameType>(src, sink, threaded);
			},
		};
	}

	template<typename FrameType>
	StreamSink StreamSink::create(const std::string& name) {
		return StreamSink{
			name,
			std::string{entt::type_name<FrameType>::value()},
		};
	}

} // Components

template<typename FrameType>
bool StreamManager::connect(Object src, Object sink, bool threaded) {
	auto res = std::find_if(
		_connections.cbegin(), _connections.cend(),
		[&](const auto& a) { return a->src == src && a->sink == sink; }
	);
	if (res != _connections.cend()) {
		// already exists
		return false;
	}

	auto h_src = _os.objectHandle(src);
	auto h_sink = _os.objectHandle(sink);
	if (!static_cast<bool>(h_src) || !static_cast<bool>(h_sink)) {
		// an object does not exist
		return false;
	}

	if (!h_src.all_of<Components::FrameStream2Source<FrameType>>()) {
		// src not stream source
		return false;
	}

	if (!h_sink.all_of<Components::FrameStream2Sink<FrameType>>()) {
		// sink not stream sink
		return false;
	}

	auto& src_stream = h_src.get<Components::FrameStream2Source<FrameType>>();
	auto& sink_stream = h_sink.get<Components::FrameStream2Sink<FrameType>>();

	struct inlineData : public Connection::Data {
		virtual ~inlineData(void) {}
		std::shared_ptr<FrameStream2I<FrameType>> reader;
		std::shared_ptr<FrameStream2I<FrameType>> writer;
	};

	auto our_data = std::make_unique<inlineData>();

	our_data->reader = src_stream->subscribe();
	if (!our_data->reader) {
		return false;
	}
	our_data->writer = sink_stream->subscribe();
	if (!our_data->writer) {
		return false;
	}

	_connections.push_back(std::make_unique<Connection>(
		h_src,
		h_sink,
		std::move(our_data),
		[](Connection& con) -> void {
			// there might be more stored
			for (size_t i = 0; i < 64; i++) {
				auto new_frame_opt = static_cast<inlineData*>(con.data.get())->reader->pop();
				// TODO: frame interval estimates
				if (new_frame_opt.has_value()) {
					static_cast<inlineData*>(con.data.get())->writer->push(new_frame_opt.value());
				} else {
					break;
				}
			}
		},
		[](Connection& con) -> void {
			auto* src_stream_ptr = con.src.try_get<Components::FrameStream2Source<FrameType>>();
			if (src_stream_ptr != nullptr) {
				(*src_stream_ptr)->unsubscribe(static_cast<inlineData*>(con.data.get())->reader);
			}
			auto* sink_stream_ptr = con.sink.try_get<Components::FrameStream2Sink<FrameType>>();
			if (sink_stream_ptr != nullptr) {
				(*sink_stream_ptr)->unsubscribe(static_cast<inlineData*>(con.data.get())->writer);
			}
		},
		!threaded
	));

	return true;
}

