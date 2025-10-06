/**
 * @file addressing.h
 * @brief RPC addressing constants and utilities
 * 
 * This header defines addressing constants used in the LitePB RPC system.
 * The RPC layer supports flexible addressing modes including direct addressing,
 * broadcast messages, and wildcard listeners.
 * 
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#ifdef LITEPB_WITH_RPC

#include <cstdint>

namespace litepb {

/**
 * @brief Wildcard address for accepting messages from any source
 * 
 * When used as a destination address in pending calls, accepts responses
 * from any node. When used in handlers, accepts requests from any source.
 * 
 * @example
 * @code{.cpp}
 * // Send request accepting response from any node
 * client.CallMethod(request, callback, timeout, RPC_ADDRESS_WILDCARD);
 * 
 * // Handler accepts requests from any source
 * channel.on_internal(service_id, method_id, 
 *     [](uint64_t src, const Request& req) {
 *         // src contains actual source address
 *         return process_request(src, req);
 *     });
 * @endcode
 */
constexpr uint64_t RPC_ADDRESS_WILDCARD  = 0x0000000000000000ULL;

/**
 * @brief Broadcast address for sending messages to all nodes
 * 
 * When used as a destination address, the message is sent to all nodes
 * on the network. Typically used for discovery, announcements, or events
 * that should be received by all participants.
 * 
 * @note Broadcast requests typically don't expect responses, or expect
 *       multiple responses from different nodes
 * 
 * @example
 * @code{.cpp}
 * // Send discovery broadcast to all nodes
 * DiscoveryRequest req;
 * client.SendEvent(req, RPC_ADDRESS_BROADCAST);
 * 
 * // Send announcement to all nodes
 * StatusUpdate status;
 * channel.send_event(SERVICE_ID, METHOD_ID, status, RPC_ADDRESS_BROADCAST);
 * @endcode
 */
constexpr uint64_t RPC_ADDRESS_BROADCAST = 0xFFFFFFFFFFFFFFFFULL;

} // namespace litepb

#endif // LITEPB_WITH_RPC
