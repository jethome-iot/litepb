#ifndef LOOPBACK_TRANSPORT_H
#define LOOPBACK_TRANSPORT_H

#include "litepb/rpc/transport.h"
#include <queue>
#include <tuple>

class LoopbackTransport : public litepb::StreamTransport
{
public:
    LoopbackTransport() : peer_(nullptr) {}

    void connect_to_peer(LoopbackTransport* peer) { peer_ = peer; }

    bool send(const uint8_t* data, size_t len, uint64_t src_addr, uint64_t dst_addr, uint16_t msg_id) override
    {
        if (!peer_) {
            return false;
        }

        // Store the addressing information
        peer_->pending_src_addr_ = src_addr;
        peer_->pending_dst_addr_ = dst_addr;
        peer_->pending_msg_id_ = msg_id;
        
        // Queue the data
        for (size_t i = 0; i < len; ++i) {
            peer_->rx_queue_.push(data[i]);
        }
        return true;
    }

    size_t recv(uint8_t* buffer, size_t max_len, uint64_t& src_addr, uint64_t& dst_addr, uint16_t& msg_id) override
    {
        // Return the addressing information
        src_addr = pending_src_addr_;
        dst_addr = pending_dst_addr_;
        msg_id = pending_msg_id_;
        
        size_t count = 0;
        while (!rx_queue_.empty() && count < max_len) {
            buffer[count++] = rx_queue_.front();
            rx_queue_.pop();
        }
        return count;
    }

    bool available() override { return !rx_queue_.empty(); }

private:
    LoopbackTransport* peer_;
    std::queue<uint8_t> rx_queue_;
    
    // Store addressing for the current message
    uint64_t pending_src_addr_ = 0;
    uint64_t pending_dst_addr_ = 0;
    uint16_t pending_msg_id_ = 0;
};

#endif