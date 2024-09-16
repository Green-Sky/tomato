#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/object_store/object_store.hpp>

#include <entt/core/type_info.hpp>

#include "./content/frame_stream2.hpp"

#include <vector>
#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>

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

		struct Data {
			virtual ~Data(void) {}
		};
		std::unique_ptr<Data> data; // stores reader writer type erased
		std::function<void(Connection&)> pump_fn;
		std::function<void(Connection&)> unsubscribe_fn;

		bool on_main_thread {true};
		std::atomic_bool stop {false}; // disconnect
		std::atomic_bool finished {false}; // disconnect

		// pump thread
		std::thread pump_thread;

		// frame interval counters and estimates

		Connection(void) = default;
		Connection(
			ObjectHandle src_,
			ObjectHandle sink_,
			std::unique_ptr<Data>&& data_,
			std::function<void(Connection&)>&& pump_fn_,
			std::function<void(Connection&)>&& unsubscribe_fn_,
			bool on_main_thread_ = true
		) :
			src(src_),
			sink(sink_),
			data(std::move(data_)),
			pump_fn(std::move(pump_fn_)),
			unsubscribe_fn(std::move(unsubscribe_fn_)),
			on_main_thread(on_main_thread_)
		{
			if (!on_main_thread) {
				// start thread
				pump_thread = std::thread([this](void) {
					while (!stop) {
						pump_fn(*this);
						std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
					finished = true;
				});
			}
		}
	};
	std::vector<std::unique_ptr<Connection>> _connections;

	public:
		StreamManager(ObjectStore2& os) : _os(os) {}
		virtual ~StreamManager(void) {
			// stop all connetions
			for (const auto& con : _connections) {
				con->stop = true;
				if (!con->on_main_thread) {
					con->pump_thread.join(); // we skip the finished check and wait
				}
				con->unsubscribe_fn(*con);
			}
		}

		// TODO: default typed sources and sinks

		// stream type is FrameStream2I<FrameType>
		// TODO: improve this design
		// src and sink need to be a FrameStream2MultiStream<FrameType>
		template<typename FrameType>
		bool connect(Object src, Object sink, bool threaded = true) {
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
					for (size_t i = 0; i < 10; i++) {
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

				if (!static_cast<bool>(con.src) || !static_cast<bool>(con.sink)) {
					// either side disappeard without disconnectAll
					// TODO: warn/error log
					con.stop = true;
				}

				if (con.on_main_thread) {
					con.pump_fn(con);
				}

				if (con.stop && (con.finished || con.on_main_thread)) {
					if (!con.on_main_thread) {
						assert(con.pump_thread.joinable());
						con.pump_thread.join();
					}
					con.unsubscribe_fn(con);
					it = _connections.erase(it);
				} else {
					it++;
				}
			}

			// return min over intervals instead
			return 0.01f;
		}
};

