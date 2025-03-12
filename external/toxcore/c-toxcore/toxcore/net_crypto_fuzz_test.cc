#include "net_crypto.h"

#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>

#include "../testing/fuzzing/fuzz_support.hh"
#include "../testing/fuzzing/fuzz_tox.hh"
#include "DHT.h"
#include "TCP_client.h"
#include "network.h"

namespace {

std::optional<std::tuple<IP_Port, uint8_t>> prepare(Fuzz_Data &input)
{
    IP_Port ipp;
    ip_init(&ipp.ip, true);
    ipp.port = 33445;

    CONSUME_OR_RETURN_VAL(const uint8_t *iterations_packed, input, 1, std::nullopt);
    uint8_t iterations = *iterations_packed;

    return {{ipp, iterations}};
}

void TestNetCrypto(Fuzz_Data &input)
{
    const auto prep = prepare(input);
    if (!prep.has_value()) {
        return;
    }
    const auto [ipp, iterations] = prep.value();

    // rest of the fuzz data is input for malloc and network
    Fuzz_System sys(input);

    const Ptr<Logger> logger(logger_new(sys.mem.get()), logger_kill);
    if (logger == nullptr) {
        return;
    }

    const Ptr<Networking_Core> net(new_networking_ex(logger.get(), sys.mem.get(), sys.ns.get(),
                                       &ipp.ip, ipp.port, ipp.port + 100, nullptr),
        kill_networking);
    if (net == nullptr) {
        return;
    }

    const std::unique_ptr<Mono_Time, std::function<void(Mono_Time *)>> mono_time(
        mono_time_new(
            sys.mem.get(), [](void *user_data) { return *static_cast<uint64_t *>(user_data); },
            &sys.clock),
        [mem = sys.mem.get()](Mono_Time *ptr) { mono_time_free(mem, ptr); });
    if (mono_time == nullptr) {
        return;
    }

    const Ptr<DHT> dht(new_dht(logger.get(), sys.mem.get(), sys.rng.get(), sys.ns.get(),
                           mono_time.get(), net.get(), false, false),
        kill_dht);
    if (dht == nullptr) {
        return;
    }

    const TCP_Proxy_Info proxy_info = {0};

    const Ptr<Net_Crypto> net_crypto(new_net_crypto(logger.get(), sys.mem.get(), sys.rng.get(),
                                         sys.ns.get(), mono_time.get(), dht.get(), &proxy_info),
        kill_net_crypto);
    if (net_crypto == nullptr) {
        return;
    }

    for (uint8_t i = 0; i < iterations; ++i) {
        networking_poll(net.get(), nullptr);
        do_dht(dht.get());
        do_net_crypto(net_crypto.get(), nullptr);
        // "Sleep"
        sys.clock += System::BOOTSTRAP_ITERATION_INTERVAL;
    }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    fuzz_select_target<TestNetCrypto>(data, size);
    return 0;
}
