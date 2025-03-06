#pragma once

#include <solanaceae/object_store/meta_components_file.hpp>

#include <solanaceae/contact/fwd.hpp>

#include <entt/container/dense_map.hpp>

namespace ObjectStore::Components {

	namespace Ephemeral {

		namespace File {

			struct TransferStatsSeparated {
				entt::dense_map<Contact4, TransferStats> stats;
			};

		} // File

	} // Ephemeral

} // ObjectStore::Components

#include "./os_comps_id.inl"

