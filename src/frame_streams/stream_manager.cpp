#include "./stream_manager.hpp"

StreamManager::Connection::Connection(
	ObjectHandle src_,
	ObjectHandle sink_,
	std::unique_ptr<Data>&& data_,
	std::function<void(Connection&)>&& pump_fn_,
	std::function<void(Connection&)>&& unsubscribe_fn_,
	bool on_main_thread_
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

StreamManager::StreamManager(ObjectStore2& os) : _os(os) {}

StreamManager::~StreamManager(void) {
	// stop all connetions
	for (const auto& con : _connections) {
		con->stop = true;
		if (!con->on_main_thread) {
			con->pump_thread.join(); // we skip the finished check and wait
		}
		con->unsubscribe_fn(*con);
	}
}

bool StreamManager::connect(Object src, Object sink, bool threaded) {
	auto h_src = _os.objectHandle(src);
	auto h_sink = _os.objectHandle(sink);
	if (!static_cast<bool>(h_src) || !static_cast<bool>(h_sink)) {
		// an object does not exist
		return false;
	}

	// get src and sink comps
	if (!h_src.all_of<Components::StreamSource>()) {
		// src not stream source
		return false;
	}

	if (!h_sink.all_of<Components::StreamSink>()) {
		// sink not stream sink
		return false;
	}

	const auto& ssrc = h_src.get<Components::StreamSource>();
	const auto& ssink = h_sink.get<Components::StreamSink>();

	// compare type
	if (ssrc.frame_type_name != ssink.frame_type_name) {
		return false;
	}

	// always fail in debug mode
	assert(static_cast<bool>(ssrc.connect_fn));
	if (!static_cast<bool>(ssrc.connect_fn)) {
		return false;
	}

	// use connect fn from src
	return ssrc.connect_fn(*this, src, sink, threaded);
}

bool StreamManager::disconnect(Object src, Object sink) {
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

bool StreamManager::disconnectAll(Object o) {
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
float StreamManager::tick(float) {
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
	return 2.f; // TODO: 2sec makes mainthread connections unusable
}

