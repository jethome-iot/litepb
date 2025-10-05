#ifndef LITEPB_RPC_CHANNEL_H
#define LITEPB_RPC_CHANNEL_H

#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include "litepb/litepb.h"
#include "litepb/rpc/addressing.h"
#include "litepb/rpc/error.h"
#include "litepb/rpc/framing.h"
#include "litepb/rpc/transport.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef LITEPB_RPC_INITIAL_BUFFER_SIZE
#define LITEPB_RPC_INITIAL_BUFFER_SIZE 1024
#endif

static_assert(LITEPB_RPC_INITIAL_BUFFER_SIZE > 0, "LITEPB_RPC_INITIAL_BUFFER_SIZE must be at least 1");

namespace litepb {

struct PendingKey
{
    uint64_t peer_addr;
    uint16_t service_id;
    uint16_t msg_id;

    bool operator==(const PendingKey& other) const
    {
        return peer_addr == other.peer_addr && service_id == other.service_id && msg_id == other.msg_id;
    }
};

} // namespace litepb

namespace std {
template <>
struct hash<litepb::PendingKey>
{
    size_t operator()(const litepb::PendingKey& key) const noexcept
    {
        return std::hash<uint64_t>()(key.peer_addr) ^ (std::hash<uint16_t>()(key.service_id) << 1) ^
            (std::hash<uint16_t>()(key.msg_id) << 2);
    }
};

template <>
struct hash<std::pair<uint16_t, uint32_t>>
{
    size_t operator()(const std::pair<uint16_t, uint32_t>& key) const noexcept
    {
        return std::hash<uint16_t>()(key.first) ^ (std::hash<uint32_t>()(key.second) << 1);
    }
};
} // namespace std

namespace litepb {

// Platform time function (weakly linked - can be overridden)
// Default implementation uses std::chrono::steady_clock
// Override this function to provide platform-specific time (e.g., millis() on Arduino)
// Must return monotonic milliseconds for timeout handling
uint32_t get_current_time_ms();

class RpcChannel
{
public:
    RpcChannel(Transport& transport, uint64_t local_address, uint32_t default_timeout_ms = 5000);

    void process();

    template <typename Req, typename Resp>
    bool call_internal(uint16_t service_id, uint32_t method_id, const Req& request,
                       std::function<void(const Result<Resp>&)> callback, uint32_t timeout_ms = 0, uint64_t dst_addr = 0);

    template <typename Req>
    bool send_event(uint16_t service_id, uint32_t method_id, const Req& request, uint64_t dst_addr = 0);

    template <typename Req, typename Resp>
    void on_internal(uint16_t service_id, uint32_t method_id, std::function<Result<Resp>(uint64_t, const Req&)> handler);

    template <typename Req>
    void on_event(uint16_t service_id, uint32_t method_id, std::function<void(uint64_t, const Req&)> handler);

private:
    struct PendingCall
    {
        uint16_t msg_id;
        uint64_t dst_addr;
        uint32_t deadline_ms;
        std::function<void(const std::vector<uint8_t>&, RpcError)> callback;
    };

    Transport& transport_;
    MessageIdGenerator id_gen_;
    bool is_stream_transport_;
    uint32_t default_timeout_ms_;
    uint64_t local_address_;

    std::unordered_map<PendingKey, PendingCall> pending_calls_;

    using HandlerFunction = std::function<void(const std::vector<uint8_t>&, uint16_t, uint64_t)>;
    using HandlerKey      = std::pair<uint16_t, uint32_t>; // (service_id, method_id)
    std::unordered_map<HandlerKey, HandlerFunction> handlers_;

    std::vector<uint8_t> rx_buffer_;
    size_t rx_pos_;

    void check_timeouts();
    void process_incoming_messages();
    void handle_message(const FramedMessage& msg);
};

template <typename Req, typename Resp>
bool RpcChannel::call_internal(uint16_t service_id, uint32_t method_id, const Req& request,
                               std::function<void(const Result<Resp>&)> callback, uint32_t timeout_ms, uint64_t dst_addr)
{
    uint16_t msg_id = id_gen_.generate_for(local_address_, dst_addr);

    BufferOutputStream payload_stream;

    if (!litepb::serialize(request, payload_stream)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::PARSE_ERROR;
        callback(error_result);
        return false;
    }

    FramedMessage framed_msg;
    framed_msg.src_addr   = local_address_;
    framed_msg.dst_addr   = dst_addr;
    framed_msg.msg_id     = msg_id;
    framed_msg.service_id = service_id;
    framed_msg.method_id  = method_id;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    BufferOutputStream out_stream;
    if (!encode_message(framed_msg, out_stream, is_stream_transport_)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::TRANSPORT_ERROR;
        callback(error_result);
        return false;
    }

    if (!transport_.send(out_stream.data(), out_stream.size())) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::TRANSPORT_ERROR;
        callback(error_result);
        return false;
    }

    uint32_t actual_timeout = (timeout_ms == 0) ? default_timeout_ms_ : timeout_ms;

    uint64_t peer_addr = (dst_addr == RPC_ADDRESS_WILDCARD || dst_addr == RPC_ADDRESS_BROADCAST) ? RPC_ADDRESS_WILDCARD : dst_addr;

    pending_calls_[PendingKey{ peer_addr, service_id, msg_id }] =
        PendingCall{ msg_id, peer_addr, get_current_time_ms() + actual_timeout,
                     [callback](const std::vector<uint8_t>& payload, RpcError error) {
                         if (!error.ok()) {
                             Result<Resp> result;
                             result.error = error;
                             callback(result);
                             return;
                         }

                         BufferInputStream in_stream(payload.data(), payload.size());
                         ProtoReader reader(in_stream);

                         uint64_t error_code_u64, app_code_u64;
                         if (!reader.read_varint(error_code_u64)) {
                             Result<Resp> result;
                             result.error.code = RpcError::PARSE_ERROR;
                             callback(result);
                             return;
                         }
                         if (!reader.read_varint(app_code_u64)) {
                             Result<Resp> result;
                             result.error.code = RpcError::PARSE_ERROR;
                             callback(result);
                             return;
                         }

                         RpcError::Code error_code = static_cast<RpcError::Code>(static_cast<uint32_t>(error_code_u64));
                         int32_t app_code          = static_cast<int32_t>(static_cast<uint32_t>(app_code_u64));

                         Resp response;
                         if (!litepb::parse(response, in_stream)) {
                             Result<Resp> result;
                             result.error.code = RpcError::PARSE_ERROR;
                             callback(result);
                             return;
                         }

                         Result<Resp> result;
                         result.value          = response;
                         result.error.code     = error_code;
                         result.error.app_code = app_code;
                         callback(result);
                     } };

    return true;
}

template <typename Req>
bool RpcChannel::send_event(uint16_t service_id, uint32_t method_id, const Req& request, uint64_t dst_addr)
{
    BufferOutputStream payload_stream;

    if (!litepb::serialize(request, payload_stream)) {
        return false;
    }

    FramedMessage framed_msg;
    framed_msg.src_addr   = local_address_;
    framed_msg.dst_addr   = dst_addr;
    framed_msg.msg_id     = 0;
    framed_msg.service_id = service_id;
    framed_msg.method_id  = method_id;
    framed_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

    BufferOutputStream out_stream;
    if (!encode_message(framed_msg, out_stream, is_stream_transport_)) {
        return false;
    }

    return transport_.send(out_stream.data(), out_stream.size());
}

template <typename Req, typename Resp>
void RpcChannel::on_internal(uint16_t service_id, uint32_t method_id, std::function<Result<Resp>(uint64_t, const Req&)> handler)
{
    handlers_[HandlerKey{ service_id, method_id }] = [this, service_id, method_id, handler](const std::vector<uint8_t>& payload,
                                                                                            uint16_t msg_id, uint64_t src_addr) {
        BufferInputStream in_stream(payload.data(), payload.size());

        Req request;
        if (!litepb::parse(request, in_stream)) {
            return;
        }

        Result<Resp> result = handler(src_addr, request);

        BufferOutputStream payload_stream;
        ProtoWriter writer(payload_stream);

        if (!writer.write_varint(static_cast<uint32_t>(result.error.code))) {
            return;
        }
        if (!writer.write_varint(static_cast<uint32_t>(result.error.app_code))) {
            return;
        }

        if (!litepb::serialize(result.value, payload_stream)) {
            return;
        }

        FramedMessage response_msg;
        response_msg.src_addr   = local_address_;
        response_msg.dst_addr   = src_addr;
        response_msg.msg_id     = msg_id;
        response_msg.service_id = service_id;
        response_msg.method_id  = method_id;
        response_msg.payload.assign(payload_stream.data(), payload_stream.data() + payload_stream.size());

        BufferOutputStream out_stream;
        if (encode_message(response_msg, out_stream, is_stream_transport_)) {
            transport_.send(out_stream.data(), out_stream.size());
        }
    };
}

template <typename Req>
void RpcChannel::on_event(uint16_t service_id, uint32_t method_id, std::function<void(uint64_t, const Req&)> handler)
{
    handlers_[HandlerKey{ service_id, method_id }] = [handler](const std::vector<uint8_t>& payload, uint16_t msg_id,
                                                               uint64_t src_addr) {
        BufferInputStream in_stream(payload.data(), payload.size());

        Req request;
        if (!litepb::parse(request, in_stream)) {
            return;
        }

        handler(src_addr, request);
    };
}

} // namespace litepb

#endif
