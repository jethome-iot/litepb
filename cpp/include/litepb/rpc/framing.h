#ifndef LITEPB_RPC_FRAMING_H
#define LITEPB_RPC_FRAMING_H

#include "rpc_protocol.pb.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace litepb {

class OutputStream;
class InputStream;

class MessageIdGenerator
{
public:
    MessageIdGenerator();

    uint16_t generate_for(uint64_t local_addr, uint64_t dst_addr);

private:
    uint16_t counter_;
};

// Transport frame containing only the RpcMessage payload
// Addressing is handled entirely by the transport layer
struct TransportFrame
{
    std::vector<uint8_t> payload; // Serialized RpcMessage only
};

bool encode_transport_frame(const TransportFrame& frame, OutputStream& output, bool is_stream_transport);

bool decode_transport_frame(InputStream& input, TransportFrame& frame, bool is_stream_transport);

// Helper functions for RpcMessage manipulation
bool serialize_rpc_message(const rpc::RpcMessage& msg, std::vector<uint8_t>& output);

bool deserialize_rpc_message(const std::vector<uint8_t>& input, rpc::RpcMessage& msg);

} // namespace litepb

#endif
