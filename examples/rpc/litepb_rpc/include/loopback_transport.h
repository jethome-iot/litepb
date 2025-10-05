#ifndef LOOPBACK_TRANSPORT_H
#define LOOPBACK_TRANSPORT_H

#include "litepb/rpc/transport.h"
#include <queue>

class LoopbackTransport : public litepb::StreamTransport
{
public:
    LoopbackTransport() : peer_(nullptr) {}

    void connect_to_peer(LoopbackTransport* peer) { peer_ = peer; }

    bool send(const uint8_t* data, size_t len) override
    {
        if (!peer_) {
            return false;
        }

        for (size_t i = 0; i < len; ++i) {
            peer_->rx_queue_.push(data[i]);
        }
        return true;
    }

    size_t recv(uint8_t* buffer, size_t max_len) override
    {
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
};

#endif
