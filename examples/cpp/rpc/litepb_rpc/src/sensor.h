#pragma once

#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "sensor.pb.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

class SensorSimulator {
public:
    SensorSimulator()
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        base_temp_ = 25.0f;
        deterministic_mode_ = false;
        deterministic_variation_ = 0.0f;
    }

    examples::sensor::ReadingResponse read_sensor(int32_t sensor_id)
    {
        examples::sensor::ReadingResponse response;
        response.sensor_id = sensor_id;

        float variation;
        if (deterministic_mode_) {
            variation = deterministic_variation_;
        } else {
            variation = (std::rand() % 100) / 10.0f - 5.0f;
        }
        response.temperature = base_temp_ + variation;

        if (response.temperature > 80.0f) {
            response.status = examples::sensor::SensorStatus::ERROR;
        } else if (response.temperature > 60.0f) {
            response.status = examples::sensor::SensorStatus::WARNING;
        } else {
            response.status = examples::sensor::SensorStatus::OK;
        }

        if (!deterministic_mode_) {
            std::cout << "  [Sensor Simulator] Reading from sensor " << sensor_id << ": " << response.temperature
                      << "°C (Status: " << static_cast<int>(response.status) << ")" << std::endl;
        }

        return response;
    }

    void simulate_temperature_spike() { base_temp_ = 85.0f; }

    void reset_temperature() { base_temp_ = 25.0f; }

    void set_deterministic_variation(float variation)
    {
        deterministic_mode_ = true;
        deterministic_variation_ = variation;
    }

private:
    float base_temp_;
    bool deterministic_mode_;
    float deterministic_variation_;
};

class SensorServiceImpl : public examples::sensor::SensorServiceServer {
public:
    SensorServiceImpl(litepb::RpcChannel& channel, SensorSimulator & simulator)
        : SensorServiceServer(channel), simulator_(simulator)
    {
    }

    litepb::Result<examples::sensor::ReadingResponse> handleGetReading(const examples::sensor::ReadingRequest & request) override
    {
        std::cout << "  [Peer B] Received GetReading request for sensor_id=" << request.sensor_id << std::endl;

        litepb::Result<examples::sensor::ReadingResponse> result;
        result.value = simulator_.read_sensor(request.sensor_id);
        result.error.code = litepb::RpcError::OK;

        std::cout << "  [Peer B] Sending response: temp=" << result.value.temperature
                  << ", status=" << static_cast<int>(result.value.status) << std::endl;

        return result;
    }

    litepb::Result<examples::sensor::AlertAck> handleNotifyAlert(const examples::sensor::AlertEvent & request) override
    {
        std::cout << "  [Peer A] ALERT RECEIVED from sensor " << request.sensor_id << ": " << request.message << std::endl;
        std::cout << "  [Peer A]   Temperature: " << request.temperature << ", Status: " << static_cast<int>(request.status)
                  << std::endl;

        litepb::Result<examples::sensor::AlertAck> result;
        result.value.received = true;
        result.error.code = litepb::RpcError::OK;

        return result;
    }

private:
    SensorSimulator & simulator_;
};

