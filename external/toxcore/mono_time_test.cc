#include "./c-toxcore/toxcore/mono_time.h"

#ifdef NDEBUG
	#undef NDEBUG
	#include <cassert>
#else
	#include <cassert>
#endif

#include <iostream>
#include <thread>
#include <chrono>

#define TEST(x, y) bool run_test_##x##y(void)

#define ASSERT_NE(x, y) assert((x) != (y))
#define EXPECT_GT(x, y) assert((x) > (y))
#define EXPECT_FALSE(x) assert(!(x))
#define EXPECT_TRUE(x) assert((x))
#define EXPECT_EQ(x, y) assert((x) == (y))

TEST(MonoTime, UnixTimeIncreasesOverTime)
{
    const Memory *mem = system_memory();
    Mono_Time *mono_time = mono_time_new(mem, nullptr, nullptr);
    ASSERT_NE(mono_time, nullptr);

    mono_time_update(mono_time);
    uint64_t const start = mono_time_get(mono_time);

    while (start == mono_time_get(mono_time)) {
        mono_time_update(mono_time);
    }

    uint64_t const end = mono_time_get(mono_time);
    EXPECT_GT(end, start);

    mono_time_free(mem, mono_time);

	return true;
}

TEST(MonoTime, IsTimeout)
{
    const Memory *mem = system_memory();
    Mono_Time *mono_time = mono_time_new(mem, nullptr, nullptr);
    ASSERT_NE(mono_time, nullptr);

    uint64_t const start = mono_time_get(mono_time);
    EXPECT_FALSE(mono_time_is_timeout(mono_time, start, 1));

    while (start == mono_time_get(mono_time)) {
        mono_time_update(mono_time);
    }

    EXPECT_TRUE(mono_time_is_timeout(mono_time, start, 1));

    mono_time_free(mem, mono_time);

	return true;
}

TEST(MonoTime, IsTimeoutReal)
{
    const Memory *mem = system_memory();
    Mono_Time *mono_time = mono_time_new(mem, nullptr, nullptr);
    ASSERT_NE(mono_time, nullptr);

    uint64_t const start = mono_time_get(mono_time);
    EXPECT_FALSE(mono_time_is_timeout(mono_time, start, 5));

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	mono_time_update(mono_time);

	// should still not have timed out (5sec) after sleeping ~20ms
    EXPECT_FALSE(mono_time_is_timeout(mono_time, start, 5));

    mono_time_free(mem, mono_time);

	return true;
}

TEST(MonoTime, CustomTime)
{
    const Memory *mem = system_memory();
    Mono_Time *mono_time = mono_time_new(mem, nullptr, nullptr);
    ASSERT_NE(mono_time, nullptr);

    uint64_t test_time = current_time_monotonic(mono_time) + 42137;

    mono_time_set_current_time_callback(
        mono_time, [](void *user_data) { return *static_cast<uint64_t *>(user_data); }, &test_time);
    mono_time_update(mono_time);

    EXPECT_EQ(current_time_monotonic(mono_time), test_time);

    uint64_t const start = mono_time_get(mono_time);

    test_time += 7000;

    mono_time_update(mono_time);
    EXPECT_EQ(mono_time_get(mono_time) - start, 7);

    EXPECT_EQ(current_time_monotonic(mono_time), test_time);

    mono_time_free(mem, mono_time);

	return true;
}

int main(int argc, char **argv) {
	if (!run_test_MonoTimeUnixTimeIncreasesOverTime()) {
		std::cerr << "MonoTimeUnixTimeIncreasesOverTime() failed\n";
		return -1;
	}
	std::cout << "MonoTimeUnixTimeIncreasesOverTime() succeeded\n";

	if (!run_test_MonoTimeIsTimeout()) {
		std::cerr << "MonoTimeIsTimeout() failed\n";
		return -1;
	}
	std::cout << "MonoTimeIsTimeout() succeeded\n";

	if (!run_test_MonoTimeIsTimeoutReal()) {
		std::cerr << "MonoTimeIsTimeoutReal() failed\n";
		return -1;
	}
	std::cout << "MonoTimeIsTimeoutReal() succeeded\n";

	if (!run_test_MonoTimeCustomTime()) {
		std::cerr << "MonoTimeCustomTime() failed\n";
		return -1;
	}
	std::cout << "MonoTimeCustomTime() succeeded\n";

	return 0;
}

