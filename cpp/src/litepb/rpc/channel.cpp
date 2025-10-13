#ifdef LITEPB_WITH_RPC

#include "litepb/rpc/channel.h"
#include <chrono>
#include <cstring>
#include <limits>

namespace litepb {

__attribute__((weak)) uint32_t get_current_time_ms()
{
    auto now = std::chrono::steady_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<uint32_t>(ms.count());
}

RpcChannel::RpcChannel(Transport& transport, uint32_t default_timeout_ms) :
    transport_(transport), id_gen_(), is_stream_transport_(dynamic_cast<StreamTransport*>(&transport) != nullptr),
    default_timeout_ms_(default_timeout_ms), rx_pos_(0)
{
    rx_buffer_.resize(LITEPB_RPC_INITIAL_BUFFER_SIZE);
}

void RpcChannel::process()
{
    check_timeouts();
    process_incoming_messages();
}

void RpcChannel::check_timeouts()
{
    uint32_t now = get_current_time_ms();

    for (auto it = pending_calls_.begin(); it != pending_calls_.end();) {
        if (now >= it->second.deadline_ms) {
            // Create timeout response
            rpc::RpcResponse timeout_response;
            timeout_response.error_code = rpc::RpcResponse::RpcErrorCode::TIMEOUT;

            it->second.callback(timeout_response);
            it = pending_calls_.erase(it);
        }
        else {
            ++it;
        }
    }
}

void RpcChannel::process_incoming_messages()
{
    if (!transport_.available()) {
        return;
    }

    while (transport_.available()) {
        if (rx_pos_ >= rx_buffer_.size()) {
            size_t new_size = rx_buffer_.size() * 2;
            if (new_size < rx_buffer_.size()) {
                new_size = std::numeric_limits<size_t>::max();
            }
            rx_buffer_.resize(new_size);
        }

        size_t received = transport_.recv(rx_buffer_.data() + rx_pos_, rx_buffer_.size() - rx_pos_);
        if (received == 0) {
            break;
        }
        rx_pos_ += received;

        BufferInputStream input(rx_buffer_.data(), rx_pos_);
        TransportFrame frame;

        if (decode_transport_frame(input, frame, is_stream_transport_)) {
            handle_message(frame);

            size_t consumed = input.position();
            if (consumed > 0 && consumed <= rx_pos_) {
                std::memmove(rx_buffer_.data(), rx_buffer_.data() + consumed, rx_pos_ - consumed);
                rx_pos_ -= consumed;
            }
        }
        else {
            break;
        }
    }
}

void RpcChannel::handle_message(const TransportFrame& frame)
{
    // Deserialize RPC message
    rpc::RpcMessage rpc_msg;
    if (!deserialize_rpc_message(frame.payload, rpc_msg)) {
        return;
    }

    // Check protocol version
    if (rpc_msg.version != 1) {
        // Unsupported protocol version
        return;
    }

    // Extract msg_id from RpcMessage
    uint16_t msg_id = static_cast<uint16_t>(rpc_msg.msg_id);

    // Handle based on message type
    switch (rpc_msg.message_type) {
    case rpc::RpcMessage::MessageType::REQUEST: {
        // This is a request, find handler
        auto handler_it = handlers_.find(HandlerKey{ static_cast<uint16_t>(rpc_msg.service_id), rpc_msg.method_id });
        if (handler_it != handlers_.end()) {
            // Extract request data
            if (std::holds_alternative<std::vector<uint8_t>>(rpc_msg.payload)) {
                const auto& request_data = std::get<std::vector<uint8_t>>(rpc_msg.payload);
                handler_it->second(request_data, msg_id);
            }
        }
        else {
            // No handler found, send error response
            rpc::RpcResponse response;
            response.error_code = rpc::RpcResponse::RpcErrorCode::HANDLER_NOT_FOUND;

            rpc::RpcMessage response_msg;
            response_msg.version      = 1;
            response_msg.service_id   = rpc_msg.service_id;
            response_msg.method_id    = 0;
            response_msg.message_type = rpc::RpcMessage::MessageType::RESPONSE;
            response_msg.msg_id       = rpc_msg.msg_id; // Echo back the msg_id for correlation
            response_msg.payload      = response;

            std::vector<uint8_t> rpc_payload;
            if (serialize_rpc_message(response_msg, rpc_payload)) {
                TransportFrame response_frame;
                response_frame.payload = rpc_payload;

                BufferOutputStream out_stream;
                if (encode_transport_frame(response_frame, out_stream, is_stream_transport_)) {
                    transport_.send(out_stream.data(), out_stream.size());
                }
            }
        }
        break;
    }

    case rpc::RpcMessage::MessageType::RESPONSE: {
        // This is a response, find pending call
        auto pending_it = pending_calls_.find(PendingKey{ static_cast<uint16_t>(rpc_msg.service_id), msg_id });
        
        if (pending_it != pending_calls_.end()) {
            // Extract response
            if (std::holds_alternative<rpc::RpcResponse>(rpc_msg.payload)) {
                const auto& response = std::get<rpc::RpcResponse>(rpc_msg.payload);
                pending_it->second.callback(response);
                pending_calls_.erase(pending_it);
            }
        }
        break;
    }

    case rpc::RpcMessage::MessageType::EVENT: {
        // This is an event, find handler
        auto handler_it = handlers_.find(HandlerKey{ static_cast<uint16_t>(rpc_msg.service_id), rpc_msg.method_id });
        if (handler_it != handlers_.end()) {
            // Extract event data
            if (std::holds_alternative<std::vector<uint8_t>>(rpc_msg.payload)) {
                const auto& event_data = std::get<std::vector<uint8_t>>(rpc_msg.payload);
                handler_it->second(event_data, 0); // msg_id = 0 for events
            }
        }
        // Events don't send responses
        break;
    }
    }
}

} // namespace litepb

#endif // LITEPB_WITH_RPC