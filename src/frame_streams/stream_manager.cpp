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

StreamManager::StreamManager(ObjectStore2& os) : _os(os), _os_sr(_os.newSubRef(this)) {
	_os_sr
		.subscribe(ObjectStore_Event::object_construct)
		//.subscribe(ObjectStore_Event::object_update)
		.subscribe(ObjectStore_Event::object_destroy)
	;
}

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
	float interval_min {2.01f};

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
			const float con_interval = con.interval_avg;
			if (con_interval > 0.f) {
				interval_min = std::min(interval_min, con_interval);
			}
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

	return interval_min;
}

bool StreamManager::onEvent(const ObjectStore::Events::ObjectConstruct& e) {
	if (!e.e.any_of<Components::StreamSink, Components::StreamSource>()) {
		return false;
	}

	// update default targets
	if (e.e.all_of<Components::TagDefaultTarget>()) {
		if (e.e.all_of<Components::StreamSource>()) {
			_default_sources[e.e.get<Components::StreamSource>().frame_type_name] = e.e;
		} else { // sink
			_default_sinks[e.e.get<Components::StreamSink>().frame_type_name] = e.e;
		}
	}

	// connect to default
	// only ever do this on new objects
	if (e.e.all_of<Components::TagConnectToDefault>()) {
		if (e.e.all_of<Components::StreamSource>()) {
			auto it_d_sink = _default_sinks.find(e.e.get<Components::StreamSource>().frame_type_name);
			if (it_d_sink != _default_sinks.cend()) {
				// TODO: threaded
				connect(e.e, it_d_sink->second);
			}
		} else { // sink
			auto it_d_src = _default_sources.find(e.e.get<Components::StreamSink>().frame_type_name);
			if (it_d_src != _default_sources.cend()) {
				// TODO: threaded
				connect(it_d_src->second, e.e);
			}
		}
	}

	return false;
}

bool StreamManager::onEvent(const ObjectStore::Events::ObjectUpdate&) {
	// what do we do here?
	return false;
}

bool StreamManager::onEvent(const ObjectStore::Events::ObjectDestory& e) {
	// typeless
	for (auto it = _default_sources.cbegin(); it != _default_sources.cend();) {
		if (it->second == e.e) {
			it = _default_sources.erase(it);
		} else {
			it++;
		}
	}
	for (auto it = _default_sinks.cbegin(); it != _default_sinks.cend();) {
		if (it->second == e.e) {
			it = _default_sinks.erase(it);
		} else {
			it++;
		}
	}

	// TODO: destroy connections
	// TODO: auto reconnect default following devices if another default exists

	return false;
}

