#include "./fragment_store_i.hpp"

#include <iostream>

void FragmentStoreI::throwEventConstruct(const FragmentID fid) {
	std::cout << "FSI debug: event construct " << entt::to_integral(fid) << "\n";
	dispatch(
		FragmentStore_Event::fragment_construct,
		Fragment::Events::FragmentConstruct{
			FragmentHandle{_reg, fid}
		}
	);
}

void FragmentStoreI::throwEventUpdate(const FragmentID fid) {
	std::cout << "FSI debug: event updated " << entt::to_integral(fid) << "\n";
	dispatch(
		FragmentStore_Event::fragment_updated,
		Fragment::Events::FragmentUpdated{
			FragmentHandle{_reg, fid}
		}
	);
}
