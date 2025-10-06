#pragma once

#ifdef LITEPB_WITH_RPC

#include <cstdint>

namespace litepb {

constexpr uint64_t RPC_ADDRESS_WILDCARD  = 0x0000000000000000ULL;
constexpr uint64_t RPC_ADDRESS_BROADCAST = 0xFFFFFFFFFFFFFFFFULL;

} // namespace litepb

#endif // LITEPB_WITH_RPC
