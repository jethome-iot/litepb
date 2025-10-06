#pragma once

#ifdef LITEPB_WITH_RPC

#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"
#include "rpc_protocol.pb.h"
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
        std::function<void(const rpc::RpcResponse&)> callback;
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
    void handle_message(const TransportFrame& frame, uint64_t src_addr, uint64_t dst_addr);
};

template <typename Req, typename Resp>
bool RpcChannel::call_internal(uint16_t service_id, uint32_t method_id, const Req& request,
                               std::function<void(const Result<Resp>&)> callback, uint32_t timeout_ms, uint64_t dst_addr)
{
    uint16_t msg_id = id_gen_.generate_for(local_address_, dst_addr);

    // Serialize request
    BufferOutputStream request_stream;
    if (!litepb::serialize(request, request_stream)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::PARSE_ERROR;
        callback(error_result);
        return false;
    }

    // Create RPC message
    rpc::RpcMessage rpc_msg;
    rpc_msg.version      = 1; // Protocol version 1
    rpc_msg.service_id   = service_id;
    rpc_msg.method_id    = method_id;
    rpc_msg.message_type = rpc::RpcMessage::MessageType::REQUEST;
    rpc_msg.msg_id       = msg_id; // Message ID moved to protocol buffer
    rpc_msg.payload      = std::vector<uint8_t>(request_stream.data(), request_stream.data() + request_stream.size());

    // Serialize RPC message
    std::vector<uint8_t> rpc_payload;
    if (!serialize_rpc_message(rpc_msg, rpc_payload)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::PARSE_ERROR;
        callback(error_result);
        return false;
    }

    // Create transport frame (only payload, addressing handled by transport)
    TransportFrame frame;
    frame.payload = rpc_payload;

    // Encode and send
    BufferOutputStream out_stream;
    if (!encode_transport_frame(frame, out_stream, is_stream_transport_)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::TRANSPORT_ERROR;
        callback(error_result);
        return false;
    }

    if (!transport_.send(out_stream.data(), out_stream.size(), local_address_, dst_addr)) {
        Result<Resp> error_result;
        error_result.error.code = RpcError::TRANSPORT_ERROR;
        callback(error_result);
        return false;
    }

    // Store pending call
    uint32_t actual_timeout = (timeout_ms == 0) ? default_timeout_ms_ : timeout_ms;
    uint64_t peer_addr = (dst_addr == RPC_ADDRESS_WILDCARD || dst_addr == RPC_ADDRESS_BROADCAST) ? RPC_ADDRESS_WILDCARD : dst_addr;

    pending_calls_[PendingKey{ peer_addr, service_id, msg_id }] =
        PendingCall{ msg_id, peer_addr, get_current_time_ms() + actual_timeout, [callback](const rpc::RpcResponse& response) {
                        Result<Resp> result;

                        // Convert RPC error code to our error code
                        switch (response.error_code) {
                        case rpc::RpcResponse::RpcErrorCode::OK:
                            result.error.code = RpcError::OK;
                            break;
                        case rpc::RpcResponse::RpcErrorCode::TIMEOUT:
                            result.error.code = RpcError::TIMEOUT;
                            break;
                        case rpc::RpcResponse::RpcErrorCode::PARSE_ERROR:
                            result.error.code = RpcError::PARSE_ERROR;
                            break;
                        case rpc::RpcResponse::RpcErrorCode::TRANSPORT_ERROR:
                            result.error.code = RpcError::TRANSPORT_ERROR;
                            break;
                        case rpc::RpcResponse::RpcErrorCode::HANDLER_NOT_FOUND:
                            result.error.code = RpcError::HANDLER_NOT_FOUND;
                            break;
                        default:
                            result.error.code = RpcError::UNKNOWN;
                            break;
                        }

                        // Parse response if no error
                        if (result.error.code == RpcError::OK && !response.response_data.empty()) {
                            BufferInputStream resp_stream(response.response_data.data(), response.response_data.size());
                            if (!litepb::parse(result.value, resp_stream)) {
                                result.error.code = RpcError::PARSE_ERROR;
                            }
                        }

                        callback(result);
                    } };

    return true;
}

template <typename Req>
bool RpcChannel::send_event(uint16_t service_id, uint32_t method_id, const Req& request, uint64_t dst_addr)
{
    // Events use msg_id = 0 (no response expected)
    uint16_t msg_id = 0;

    // Serialize request
    BufferOutputStream request_stream;
    if (!litepb::serialize(request, request_stream)) {
        return false;
    }

    // Create RPC message
    rpc::RpcMessage rpc_msg;
    rpc_msg.version      = 1; // Protocol version 1
    rpc_msg.service_id   = service_id;
    rpc_msg.method_id    = method_id;
    rpc_msg.message_type = rpc::RpcMessage::MessageType::EVENT;
    rpc_msg.msg_id       = msg_id; // Events use msg_id = 0
    rpc_msg.payload      = std::vector<uint8_t>(request_stream.data(), request_stream.data() + request_stream.size());

    // Serialize RPC message
    std::vector<uint8_t> rpc_payload;
    if (!serialize_rpc_message(rpc_msg, rpc_payload)) {
        return false;
    }

    // Create transport frame (only payload, addressing handled by transport)
    TransportFrame frame;
    frame.payload = rpc_payload;

    // Encode and send
    BufferOutputStream out_stream;
    if (!encode_transport_frame(frame, out_stream, is_stream_transport_)) {
        return false;
    }

    return transport_.send(out_stream.data(), out_stream.size(), local_address_, dst_addr);
}

template <typename Req, typename Resp>
void RpcChannel::on_internal(uint16_t service_id, uint32_t method_id, std::function<Result<Resp>(uint64_t, const Req&)> handler)
{
    handlers_[HandlerKey{ service_id, method_id }] = [handler, this, service_id](const std::vector<uint8_t>& request_data,
                                                                                 uint16_t msg_id, uint64_t src_addr) {
        // Parse request
        Req request;
        BufferInputStream req_stream(request_data.data(), request_data.size());
        if (!litepb::parse(request, req_stream)) {
            // Send error response
            rpc::RpcResponse response;
            response.error_code = rpc::RpcResponse::RpcErrorCode::PARSE_ERROR;

            rpc::RpcMessage rpc_msg;
            rpc_msg.version      = 1;
            rpc_msg.service_id   = service_id;
            rpc_msg.method_id    = 0; // Method ID not relevant for response
            rpc_msg.message_type = rpc::RpcMessage::MessageType::RESPONSE;
            rpc_msg.msg_id       = msg_id; // Include msg_id for correlation
            rpc_msg.payload      = response;

            std::vector<uint8_t> rpc_payload;
            if (serialize_rpc_message(rpc_msg, rpc_payload)) {
                TransportFrame frame;
                frame.payload = rpc_payload;

                BufferOutputStream out_stream;
                if (encode_transport_frame(frame, out_stream, is_stream_transport_)) {
                    transport_.send(out_stream.data(), out_stream.size(), local_address_, src_addr);
                }
            }
            return;
        }

        // Call handler
        Result<Resp> result = handler(src_addr, request);

        // Create response
        rpc::RpcResponse response;

        // Map our error codes to RPC error codes
        switch (result.error.code) {
        case RpcError::OK:
            response.error_code = rpc::RpcResponse::RpcErrorCode::OK;
            break;
        case RpcError::TIMEOUT:
            response.error_code = rpc::RpcResponse::RpcErrorCode::TIMEOUT;
            break;
        case RpcError::PARSE_ERROR:
            response.error_code = rpc::RpcResponse::RpcErrorCode::PARSE_ERROR;
            break;
        case RpcError::TRANSPORT_ERROR:
            response.error_code = rpc::RpcResponse::RpcErrorCode::TRANSPORT_ERROR;
            break;
        case RpcError::HANDLER_NOT_FOUND:
            response.error_code = rpc::RpcResponse::RpcErrorCode::HANDLER_NOT_FOUND;
            break;
        default:
            response.error_code = rpc::RpcResponse::RpcErrorCode::TRANSPORT_ERROR;
            break;
        }

        // Serialize response value if successful
        if (response.error_code == rpc::RpcResponse::RpcErrorCode::OK) {
            BufferOutputStream resp_stream;
            if (litepb::serialize(result.value, resp_stream)) {
                response.response_data.assign(resp_stream.data(), resp_stream.data() + resp_stream.size());
            }
            else {
                response.error_code = rpc::RpcResponse::RpcErrorCode::PARSE_ERROR;
            }
        }

        // Create RPC message
        rpc::RpcMessage rpc_msg;
        rpc_msg.version      = 1;
        rpc_msg.service_id   = service_id;
        rpc_msg.method_id    = 0; // Method ID not relevant for response
        rpc_msg.message_type = rpc::RpcMessage::MessageType::RESPONSE;
        rpc_msg.msg_id       = msg_id; // Include msg_id for correlation
        rpc_msg.payload      = response;

        // Serialize and send
        std::vector<uint8_t> rpc_payload;
        if (serialize_rpc_message(rpc_msg, rpc_payload)) {
            TransportFrame frame;
            frame.payload = rpc_payload;

            BufferOutputStream out_stream;
            if (encode_transport_frame(frame, out_stream, is_stream_transport_)) {
                transport_.send(out_stream.data(), out_stream.size(), local_address_, src_addr);
            }
        }
    };
}

template <typename Req>
void RpcChannel::on_event(uint16_t service_id, uint32_t method_id, std::function<void(uint64_t, const Req&)> handler)
{
    handlers_[HandlerKey{ service_id, method_id }] = [handler](const std::vector<uint8_t>& event_data, uint16_t msg_id,
                                                               uint64_t src_addr) {
        // Parse event
        Req event;
        BufferInputStream event_stream(event_data.data(), event_data.size());
        if (litepb::parse(event, event_stream)) {
            handler(src_addr, event);
        }
        // Events don't send responses
    };
}

} // namespace litepb

#endif // LITEPB_WITH_RPC
