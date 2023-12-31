#include "../toxcore/tox.h"
#include "check_compat.h"

#define CHECK(major, minor, patch, expected)                            \
  do_check(TOX_VERSION_MAJOR, TOX_VERSION_MINOR, TOX_VERSION_PATCH,     \
           major, minor, patch,                                         \
           TOX_VERSION_IS_API_COMPATIBLE(major, minor, patch), expected)

static void do_check(int lib_major, int lib_minor, int lib_patch,
                     int cli_major, int cli_minor, int cli_patch,
                     bool actual, bool expected)
{
    ck_assert_msg(actual == expected,
                  "Client version %d.%d.%d is%s compatible with library version %d.%d.%d, but it should%s be\n",
                  cli_major, cli_minor, cli_patch, actual ? "" : " not",
                  lib_major, lib_minor, lib_patch, expected ? "" : " not");
}

#undef TOX_VERSION_MAJOR
#undef TOX_VERSION_MINOR
#undef TOX_VERSION_PATCH

int main(void)
{
#define TOX_VERSION_MAJOR 0
#define TOX_VERSION_MINOR 0
#define TOX_VERSION_PATCH 4
    // Tox versions from 0.0.* are only compatible with themselves.
    CHECK(0, 0, 0, false);
    CHECK(0, 0, 3, false);
    CHECK(0, 0, 4, true);
    CHECK(0, 0, 5, false);
    CHECK(1, 0, 4, false);
#undef TOX_VERSION_MAJOR
#undef TOX_VERSION_MINOR
#undef TOX_VERSION_PATCH

#define TOX_VERSION_MAJOR 0
#define TOX_VERSION_MINOR 1
#define TOX_VERSION_PATCH 4
    // Tox versions from 0.1.* are only compatible with themselves or 0.1.<*
    CHECK(0, 0, 0, false);
    CHECK(0, 0, 4, false);
    CHECK(0, 0, 5, false);
    CHECK(0, 1, 0, true);
    CHECK(0, 1, 4, true);
    CHECK(0, 1, 5, false);
    CHECK(0, 2, 0, false);
    CHECK(0, 2, 4, false);
    CHECK(0, 2, 5, false);
    CHECK(1, 0, 0, false);
    CHECK(1, 0, 4, false);
    CHECK(1, 0, 5, false);
    CHECK(1, 1, 4, false);
#undef TOX_VERSION_MAJOR
#undef TOX_VERSION_MINOR
#undef TOX_VERSION_PATCH

#define TOX_VERSION_MAJOR 1
#define TOX_VERSION_MINOR 0
#define TOX_VERSION_PATCH 4
    // Beyond 0.*.* Tox is comfortable with any lower version within their major
    CHECK(0, 0, 4, false);
    CHECK(1, 0, 0, true);
    CHECK(1, 0, 1, true);
    CHECK(1, 0, 4, true);
    CHECK(1, 0, 5, false);
    CHECK(1, 1, 0, false);
    CHECK(2, 0, 0, false);
    CHECK(2, 0, 4, false);
#undef TOX_VERSION_MAJOR
#undef TOX_VERSION_MINOR
#undef TOX_VERSION_PATCH

#define TOX_VERSION_MAJOR 1
#define TOX_VERSION_MINOR 1
#define TOX_VERSION_PATCH 4
    CHECK(0, 0, 4, false);
    CHECK(1, 0, 0, true);
    CHECK(1, 0, 4, true);
    CHECK(1, 0, 5, true);
    CHECK(1, 1, 0, true);
    CHECK(1, 1, 1, true);
    CHECK(1, 1, 4, true);
    CHECK(1, 1, 5, false);
    CHECK(1, 2, 0, false);
    CHECK(1, 2, 4, false);
    CHECK(1, 2, 5, false);
    CHECK(2, 0, 0, false);
    CHECK(2, 1, 4, false);
#undef TOX_VERSION_MAJOR
#undef TOX_VERSION_MINOR
#undef TOX_VERSION_PATCH

    return 0;
}
