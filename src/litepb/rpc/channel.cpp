#include "litepb/rpc/channel.h"
#include "litepb/rpc/addressing.h"
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

RpcChannel::RpcChannel(Transport& transport, uint64_t local_address, uint32_t default_timeout_ms) :
    transport_(transport), id_gen_(), is_stream_transport_(dynamic_cast<StreamTransport*>(&transport) != nullptr),
    default_timeout_ms_(default_timeout_ms), local_address_(local_address), rx_pos_(0)
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
            RpcError timeout_error;
            timeout_error.code = RpcError::TIMEOUT;

            std::vector<uint8_t> empty_payload;
            it->second.callback(empty_payload, timeout_error);

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
        FramedMessage msg;

        if (decode_message(input, msg, is_stream_transport_)) {
            handle_message(msg);

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

void RpcChannel::handle_message(const FramedMessage& msg)
{
    if (msg.dst_addr != RPC_ADDRESS_WILDCARD && msg.dst_addr != local_address_ && msg.dst_addr != RPC_ADDRESS_BROADCAST) {
        return;
    }

    if (msg.msg_id == 0) {
        // Event: no response expected
        auto handler_it = handlers_.find(HandlerKey{ msg.service_id, msg.method_id });

        if (handler_it != handlers_.end()) {
            handler_it->second(msg.payload, msg.msg_id, msg.src_addr);
        }
    }
    else {
        // Normal RPC or response
        auto pending_it        = pending_calls_.find(PendingKey{ msg.src_addr, msg.service_id, msg.msg_id });
        bool is_valid_response = false;

        if (pending_it != pending_calls_.end()) {
            // Verify this is actually a response to our call
            if (pending_it->second.dst_addr == msg.src_addr) {
                is_valid_response = true;
            }
        }

        if (!is_valid_response) {
            // Try broadcast fallback
            pending_it = pending_calls_.find(PendingKey{ RPC_ADDRESS_WILDCARD, msg.service_id, msg.msg_id });
            if (pending_it != pending_calls_.end() && pending_it->second.dst_addr == RPC_ADDRESS_WILDCARD) {
                is_valid_response = true;
            }
        }

        if (is_valid_response) {
            // Handle as response
            RpcError ok_error;
            ok_error.code = RpcError::OK;

            pending_it->second.callback(msg.payload, ok_error);
            pending_calls_.erase(pending_it);
        }
        else {
            // Handle as request
            auto handler_it = handlers_.find(HandlerKey{ msg.service_id, msg.method_id });

            if (handler_it != handlers_.end()) {
                handler_it->second(msg.payload, msg.msg_id, msg.src_addr);
            }
        }
    }
}

} // namespace litepb
