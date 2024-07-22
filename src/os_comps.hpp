#pragma once

#include <solanaceae/object_store/meta_components_file.hpp>

#include <solanaceae/contact/contact_model3.hpp>

#include <entt/container/dense_map.hpp>

namespace ObjectStore::Components {

	// until i find a better name
	namespace File {

		// ephemeral?, not sure saving this to disk makes sense
		// tag remove have all?
		struct RemoteHaveBitset {
			struct Entry {
				bool have_all {false};
				BitSet have;
			};
			entt::dense_map<Contact3, Entry> others;
		};

	} // File

	namespace Ephemeral {

		namespace File {

			struct TransferStatsSeparated {
				entt::dense_map<Contact3, TransferStats> stats;
			};

		} // File

	} // Ephemeral

} // ObjectStore::Components

#include "./os_comps_id.inl"

