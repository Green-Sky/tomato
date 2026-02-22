#pragma once

#include <solanaceae/contact/fwd.hpp>

struct ContactStore4Impl;


// in place
// TODO: caching variant
void renderContactInfo(
	ContactStore4Impl& cs,
	ContactHandle4 c,
	bool advanced = false,
	bool verbose = false
);

