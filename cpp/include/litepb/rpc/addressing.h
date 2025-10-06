#ifndef LITEPB_RPC_ADDRESSING_H
#define LITEPB_RPC_ADDRESSING_H

#include <cstdint>

namespace litepb {

constexpr uint64_t RPC_ADDRESS_WILDCARD  = 0x0000000000000000ULL;
constexpr uint64_t RPC_ADDRESS_BROADCAST = 0xFFFFFFFFFFFFFFFFULL;

} // namespace litepb

#endif
