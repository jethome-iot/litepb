#include "litepb/rpc/framing.h"
#include "litepb/core/streams.h"
#include "litepb/litepb.h"
#include <cstring>

namespace litepb {

MessageIdGenerator::MessageIdGenerator() : counter_(1) {}

uint16_t MessageIdGenerator::generate_for(uint64_t local_addr, uint64_t dst_addr)
{
    uint16_t id = counter_;
    counter_++;
    if (counter_ == 0) {
        counter_ = 1;  // Skip 0 as it's used for events
    }
    return id;
}

bool encode_transport_frame(const TransportFrame& frame, OutputStream& output, bool is_stream_transport)
{
    // Transport handles all addressing - we only encode the RpcMessage payload
    // Format for stream: [payload_len:varint][payload]
    // Format for packet: [payload]
    
    // For stream transports, write payload length as varint
    if (is_stream_transport) {
        ProtoWriter writer(output);
        writer.write_varint(static_cast<uint64_t>(frame.payload.size()));
    }
    
    // Write payload
    if (!frame.payload.empty()) {
        if (!output.write(frame.payload.data(), frame.payload.size())) {
            return false;
        }
    }
    
    return true;
}

bool decode_transport_frame(InputStream& input, TransportFrame& frame, bool is_stream_transport)
{
    // Transport handles all addressing - we only decode the RpcMessage payload
    // Format for stream: [payload_len:varint][payload]
    // Format for packet: [payload]
    
    // Read payload length
    uint32_t payload_len = 0;
    if (is_stream_transport) {
        ProtoReader reader(input);
        uint64_t len_varint;
        if (!reader.read_varint(len_varint)) {
            return false;
        }
        payload_len = static_cast<uint32_t>(len_varint);
    } else {
        // For packet transports, remaining data is the payload
        payload_len = static_cast<uint32_t>(input.available());
    }
    
    // Read payload
    if (payload_len > 0) {
        frame.payload.resize(payload_len);
        if (!input.read(frame.payload.data(), payload_len)) {
            return false;
        }
    } else {
        frame.payload.clear();
    }
    
    return true;
}

bool serialize_rpc_message(const rpc::RpcMessage& msg, std::vector<uint8_t>& output)
{
    output.clear();
    BufferOutputStream stream;
    if (!litepb::serialize(msg, stream)) {
        return false;
    }
    output.assign(stream.data(), stream.data() + stream.size());
    return true;
}

bool deserialize_rpc_message(const std::vector<uint8_t>& input, rpc::RpcMessage& msg)
{
    if (input.empty()) {
        return false;
    }
    BufferInputStream stream(input.data(), input.size());
    return litepb::parse(msg, stream);
}

} // namespace litepb