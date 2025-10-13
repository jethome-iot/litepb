#include "litepb/litepb.h"
#include "litepb/rpc/channel.h"
#include "litepb/rpc/transport.h"
#include "loopback_transport.h"
#include "sensor.h"
#include "switch.h"
#include <cstring>
#include <iostream>

using namespace examples::sensor;
using namespace switch_service;
using namespace litepb;

int main()
{
    std::cout << "=== Multi-Service RPC Example (Sensor + Switch) ===" << std::endl;
    std::cout << std::endl;

    LoopbackTransport peer_a_transport;
    LoopbackTransport peer_b_transport;

    peer_a_transport.connect_to_peer(&peer_b_transport);
    peer_b_transport.connect_to_peer(&peer_a_transport);

    RpcChannel peer_a_channel(peer_a_transport, 0x01, 1000);
    RpcChannel peer_b_channel(peer_b_transport, 0x02, 1000);

    std::cout << "Step 1: Setting up services on both peers..." << std::endl;
    std::cout << "  - SensorService (service_id=1)" << std::endl;
    std::cout << "  - SwitchService (service_id=2)" << std::endl;
    std::cout << std::endl;

    SensorSimulator simulator;
    SensorServiceImpl sensor_service(peer_b_channel, simulator);

    SwitchManager switch_manager;
    SwitchServiceImpl switch_service(peer_b_channel, switch_manager);

    std::cout << "Step 2: Setting up StateChanged event handler on Peer A..." << std::endl;
    peer_a_channel.on_event<StateChangedEvent>(2, 4, [](const StateChangedEvent& event) {
        std::cout << "  [Peer A] StateChanged EVENT: Switch " << event.switch_id << " is now " << (event.is_on ? "ON" : "OFF")
                  << std::endl;
        std::cout << "           Reason: " << event.reason << std::endl;
    });
    std::cout << std::endl;

    std::cout << "Step 3: Peer A calls GetReading on SensorService (service_id=1)..." << std::endl;
    SensorServiceClient sensor_client(peer_a_channel);
    bool sensor_response_received = false;
    float current_temperature     = 0.0f;

    ReadingRequest sensor_request;
    sensor_request.sensor_id = 42;

    sensor_client.GetReading(
        sensor_request, [&sensor_response_received, &current_temperature](const Result<ReadingResponse>& result) {
            if (result.ok()) {
                std::cout << "  [Peer A] GetReading SUCCESS:" << std::endl;
                std::cout << "    Sensor ID: " << result.value.sensor_id << std::endl;
                std::cout << "    Temperature: " << result.value.temperature << "°C" << std::endl;
                std::cout << "    Status: ";
                switch (result.value.status) {
                case SensorStatus::OK:
                    std::cout << "OK" << std::endl;
                    break;
                case SensorStatus::WARNING:
                    std::cout << "WARNING" << std::endl;
                    break;
                case SensorStatus::ERROR:
                    std::cout << "ERROR" << std::endl;
                    break;
                }
                current_temperature = result.value.temperature;
            }
            else {
                std::cout << "  [Peer A] GetReading FAILED (error code: " << result.error.code << ")" << std::endl;
            }
            sensor_response_received = true;
        });

    for (int i = 0; i < 20 && !sensor_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }
    std::cout << std::endl;

    std::cout << "Step 4: Temperature check - controlling switch based on sensor reading..." << std::endl;
    if (current_temperature > 60.0f) {
        std::cout << "  [Peer A] Temperature HIGH! Turning ON cooling switch..." << std::endl;

        SwitchServiceClient switch_client(peer_a_channel);
        bool switch_response_received = false;

        TurnOnRequest switch_request;
        switch_request.switch_id = 1;

        switch_client.TurnOn(switch_request, [&switch_response_received](const Result<TurnOnResponse>& result) {
            if (result.ok()) {
                std::cout << "  [Peer A] TurnOn SUCCESS:" << std::endl;
                std::cout << "    Switch is now: " << (result.value.is_on ? "ON" : "OFF") << std::endl;
            }
            else {
                std::cout << "  [Peer A] TurnOn FAILED (error code: " << result.error.code << ")" << std::endl;
            }
            switch_response_received = true;
        });

        for (int i = 0; i < 20 && !switch_response_received; ++i) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }
    else {
        std::cout << "  [Peer A] Temperature normal. No action needed." << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Step 5: Demonstrating direct switch control - Toggle switch..." << std::endl;
    SwitchServiceClient switch_client(peer_a_channel);
    bool toggle_response_received = false;

    ToggleRequest toggle_request;
    toggle_request.switch_id = 1;

    switch_client.Toggle(toggle_request, [&toggle_response_received](const Result<ToggleResponse>& result) {
        if (result.ok()) {
            std::cout << "  [Peer A] Toggle SUCCESS:" << std::endl;
            std::cout << "    Switch is now: " << (result.value.is_on ? "ON" : "OFF") << std::endl;
        }
        else {
            std::cout << "  [Peer A] Toggle FAILED (error code: " << result.error.code << ")" << std::endl;
        }
        toggle_response_received = true;
    });

    for (int i = 0; i < 20 && !toggle_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }
    std::cout << std::endl;

    std::cout << "Step 6: Peer B sends StateChanged fire-and-forget event..." << std::endl;
    
    StateChangedEvent state_event;
    state_event.switch_id = 1;
    state_event.is_on     = switch_manager.get_state(1);
    state_event.reason    = "Manual state change notification";

    bool event_sent = switch_service.emitStateChanged(state_event);
    std::cout << "  [Peer B] StateChanged event sent: " << (event_sent ? "SUCCESS" : "FAILED") << std::endl;

    for (int i = 0; i < 20; ++i) {
        peer_a_channel.process();
        peer_b_channel.process();
    }
    std::cout << std::endl;

    std::cout << "Step 7: Simulating temperature spike and automated response..." << std::endl;
    simulator.simulate_temperature_spike();

    sensor_response_received = false;
    sensor_client.GetReading(sensor_request,
                             [&sensor_response_received, &current_temperature](const Result<ReadingResponse>& result) {
                                 if (result.ok()) {
                                     std::cout << "  [Peer A] GetReading SUCCESS:" << std::endl;
                                     std::cout << "    Temperature: " << result.value.temperature << "°C (CRITICAL!)" << std::endl;
                                     current_temperature = result.value.temperature;
                                 }
                                 sensor_response_received = true;
                             });

    for (int i = 0; i < 20 && !sensor_response_received; ++i) {
        peer_b_channel.process();
        peer_a_channel.process();
    }

    if (current_temperature > 80.0f) {
        std::cout << "  [Peer A] CRITICAL TEMPERATURE! Ensuring switch is ON..." << std::endl;

        TurnOnRequest critical_switch_request;
        critical_switch_request.switch_id = 1;

        bool critical_response = false;
        switch_client.TurnOn(critical_switch_request, [&critical_response](const Result<TurnOnResponse>& result) {
            if (result.ok()) {
                std::cout << "  [Peer A] Emergency cooling activated: " << (result.value.is_on ? "ON" : "OFF") << std::endl;
            }
            critical_response = true;
        });

        for (int i = 0; i < 20 && !critical_response; ++i) {
            peer_b_channel.process();
            peer_a_channel.process();
        }
    }
    std::cout << std::endl;

    std::cout << "=== Example Complete ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrated multi-service architecture with:" << std::endl;
    std::cout << "  1. SensorService (service_id=1) - Temperature monitoring" << std::endl;
    std::cout << "  2. SwitchService (service_id=2) - Device control" << std::endl;
    std::cout << "  3. Inter-service communication - Sensor readings control switches" << std::endl;
    std::cout << "  4. Service_id routing - Multiple services on the same channel" << std::endl;
    std::cout << "  5. Modular architecture - Separate files for each service" << std::endl;
    std::cout << std::endl;
    std::cout << "Key architectural points:" << std::endl;
    std::cout << "  • Each service has its own service_id (1 for Sensor, 2 for Switch)" << std::endl;
    std::cout << "  • Services are independent but can work together" << std::endl;
    std::cout << "  • Type-safe client/server stubs for each service" << std::endl;
    std::cout << "  • Single RpcChannel handles multiple services via service_id routing" << std::endl;
    std::cout << "  • Clean separation: sensor.h/cpp, switch.h/cpp, main.cpp" << std::endl;
    std::cout << std::endl;
    std::cout << "Use cases demonstrated:" << std::endl;
    std::cout << "  • Monitoring temperature and controlling cooling based on thresholds" << std::endl;
    std::cout << "  • Direct switch control (Toggle)" << std::endl;
    std::cout << "  • State change notifications (fire-and-forget events)" << std::endl;
    std::cout << "  • Emergency response to critical conditions" << std::endl;

    return 0;
}
