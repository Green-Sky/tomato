#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/object_store/object_store.hpp>

#include <entt/core/type_info.hpp>

#include "./content/frame_stream2.hpp"

#include <vector>
#include <memory>
#include <algorithm>

namespace Components {
	struct StreamSource {
		std::string name;
		std::string frame_type_name;
		// TODO: connect fn
	};

	struct StreamSink {
		std::string name;
		std::string frame_type_name;
		// TODO: connect fn
	};

	template<typename FrameType>
	using FrameStream2Source = std::unique_ptr<FrameStream2SourceI<FrameType>>;

	template<typename FrameType>
	using FrameStream2Sink = std::unique_ptr<FrameStream2SinkI<FrameType>>;

} // Components


class StreamManager {
	friend class StreamManagerUI;
	ObjectStore2& _os;

	struct Connection {
		ObjectHandle src;
		ObjectHandle sink;

		std::function<void(Connection&)> pump_fn;

		bool on_main_thread {true};
		std::atomic_bool stop {false}; // disconnect
		std::atomic_bool finished {false}; // disconnect

		// pump thread

		// frame interval counters and estimates

		Connection(void) = default;
		Connection(
			ObjectHandle src_,
			ObjectHandle sink_,
			std::function<void(Connection&)>&& pump_fn_,
			bool on_main_thread_ = true
		) :
			src(src_),
			sink(sink_),
			pump_fn(std::move(pump_fn_)),
			on_main_thread(on_main_thread_)
		{}
	};
	std::vector<std::unique_ptr<Connection>> _connections;

	public:
		StreamManager(ObjectStore2& os) : _os(os) {}
		virtual ~StreamManager(void) {}

		// TODO: default typed sources and sinks

		// stream type is FrameStream2I<FrameType>
		// TODO: improve this design
		// src and sink need to be a FrameStream2MultiStream<FrameType>
		template<typename FrameType>
		bool connect(Object src, Object sink) {
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

			// HACK:
			if (!h_src.all_of<Components::StreamSource>()) {
				h_src.emplace<Components::StreamSource>("", std::string{entt::type_name<FrameType>::value()});
			}
			if (!h_sink.all_of<Components::StreamSink>()) {
				h_sink.emplace<Components::StreamSink>("", std::string{entt::type_name<FrameType>::value()});
			}

			auto& src_stream = h_src.get<Components::FrameStream2Source<FrameType>>();
			auto& sink_stream = h_sink.get<Components::FrameStream2Sink<FrameType>>();

			auto reader = src_stream->subscribe();
			if (!reader) {
				return false;
			}
			auto writer = sink_stream->subscribe();
			if (!writer) {
				return false;
			}

			_connections.push_back(std::make_unique<Connection>(
				h_src,
				h_sink,
				// refactor extract, we just need the type info here
				[reader = std::move(reader), writer = std::move(writer)](Connection& con) -> void {
					// there might be more stored
					for (size_t i = 0; i < 10; i++) {
						auto new_frame_opt = reader->pop();
						// TODO: frame interval estimates
						if (new_frame_opt.has_value()) {
							writer->push(new_frame_opt.value());
						} else {
							break;
						}
					}

					if (con.stop) {
						auto* src_stream_ptr = con.src.try_get<Components::FrameStream2Source<FrameType>>();
						if (src_stream_ptr != nullptr) {
							(*src_stream_ptr)->unsubscribe(reader);
						}
						auto* sink_stream_ptr = con.sink.try_get<Components::FrameStream2Sink<FrameType>>();
						if (sink_stream_ptr != nullptr) {
							(*sink_stream_ptr)->unsubscribe(writer);
						}
						con.finished = true;
					}
				},
				true // TODO: threaded
			));

			return true;
		}

		template<typename StreamType>
		bool disconnect(Object src, Object sink) {
			auto res = std::find_if(
				_connections.cbegin(), _connections.cend(),
				[&](const auto& a) { return a->src == src && a->sink == sink; }
			);
			if (res == _connections.cend()) {
				// not found
				return false;
			}

			// do disconnect
			(*res)->stop = true;

			return true;
		}

		template<typename StreamType>
		bool disconnectAll(Object o) {
			bool succ {false};
			for (const auto& con : _connections) {
				if (con->src == o || con->sink == o) {
					con->stop = true;
					succ = true;
				}
			}

			return succ;
		}

		// do we need the time delta?
		float tick(float) {
			// pump all mainthread connections
			for (auto it = _connections.begin(); it != _connections.end();) {
				auto& con = **it;
				if (con.on_main_thread) {
					con.pump_fn(con);
				}

				if (con.stop && con.finished) {
					it = _connections.erase(it);
				} else {
					it++;
				}
			}

			// return min over intervals instead
			return 0.01f;
		}
};

