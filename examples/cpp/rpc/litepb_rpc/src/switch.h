#pragma once

#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "switch.pb.h"
#include <iostream>
#include <map>

class SwitchManager {
public:
    SwitchManager() = default;

    bool turn_on(uint32_t switch_id)
    {
        std::cout << "  [Switch Manager] Turning ON switch " << switch_id << std::endl;
        switches_[switch_id] = true;
        return true;
    }

    bool turn_off(uint32_t switch_id)
    {
        std::cout << "  [Switch Manager] Turning OFF switch " << switch_id << std::endl;
        switches_[switch_id] = false;
        return true;
    }

    bool toggle(uint32_t switch_id)
    {
        bool current_state = get_state(switch_id);
        bool new_state = !current_state;
        switches_[switch_id] = new_state;
        std::cout << "  [Switch Manager] Toggling switch " << switch_id << " from " << (current_state ? "ON" : "OFF") << " to "
                  << (new_state ? "ON" : "OFF") << std::endl;
        return true;
    }

    bool get_state(uint32_t switch_id) const
    {
        auto it = switches_.find(switch_id);
        return (it != switches_.end()) ? it->second : false;
    }

private:
    std::map<uint32_t, bool> switches_;
};

class SwitchServiceImpl : public switch_service::SwitchServiceServer {
public:
    SwitchServiceImpl(SwitchManager & manager)
        : manager_(manager)
    {
    }

    litepb::Result<switch_service::TurnOnResponse> TurnOn(uint64_t src_addr, const switch_service::TurnOnRequest & request) override
    {
        std::cout << "  [Peer B] Received TurnOn request for switch_id=" << request.switch_id << std::endl;

        litepb::Result<switch_service::TurnOnResponse> result;
        result.value.success = manager_.turn_on(request.switch_id);
        result.value.is_on = manager_.get_state(request.switch_id);
        result.error.code = litepb::RpcError::OK;

        std::cout << "  [Peer B] TurnOn response: success=" << result.value.success << ", is_on=" << result.value.is_on
                  << std::endl;

        return result;
    }

    litepb::Result<switch_service::TurnOffResponse> TurnOff(uint64_t src_addr,
        const switch_service::TurnOffRequest & request) override
    {
        std::cout << "  [Peer B] Received TurnOff request for switch_id=" << request.switch_id << std::endl;

        litepb::Result<switch_service::TurnOffResponse> result;
        result.value.success = manager_.turn_off(request.switch_id);
        result.value.is_on = manager_.get_state(request.switch_id);
        result.error.code = litepb::RpcError::OK;

        std::cout << "  [Peer B] TurnOff response: success=" << result.value.success << ", is_on=" << result.value.is_on
                  << std::endl;

        return result;
    }

    litepb::Result<switch_service::ToggleResponse> Toggle(uint64_t src_addr, const switch_service::ToggleRequest & request) override
    {
        std::cout << "  [Peer B] Received Toggle request for switch_id=" << request.switch_id << std::endl;

        litepb::Result<switch_service::ToggleResponse> result;
        result.value.success = manager_.toggle(request.switch_id);
        result.value.is_on = manager_.get_state(request.switch_id);
        result.error.code = litepb::RpcError::OK;

        std::cout << "  [Peer B] Toggle response: success=" << result.value.success << ", is_on=" << result.value.is_on
                  << std::endl;

        return result;
    }

private:
    SwitchManager & manager_;
};

void setup_switch_service(litepb::RpcChannel & channel, switch_service::SwitchServiceServer & service);
