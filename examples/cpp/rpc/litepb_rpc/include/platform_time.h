#pragma once

#include <cstdint>

// Platform-specific time implementations for LitePB RPC
//
// The get_current_time_ms() function is weakly linked, allowing you to override
// it with platform-specific implementations for optimal performance.
//
// Simply define get_current_time_ms() in your application code to override
// the default std::chrono implementation.

// ============================================================================
// ARDUINO / ESP32
// ============================================================================
#ifdef ARDUINO

namespace litepb {

uint32_t get_current_time_ms()
{
    return millis();
}

} // namespace litepb

#endif // ARDUINO

// ============================================================================
// ESP-IDF (ESP32 without Arduino)
// ============================================================================
#ifdef ESP_PLATFORM
#ifndef ARDUINO

#include "esp_timer.h"

namespace litepb {

uint32_t get_current_time_ms()
{
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

} // namespace litepb

#endif // !ARDUINO
#endif // ESP_PLATFORM

// ============================================================================
// FREERTOS
// ============================================================================
#ifdef FREERTOS_ENABLED

#include "FreeRTOS.h"
#include "task.h"

namespace litepb {

uint32_t get_current_time_ms()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

} // namespace litepb

#endif // FREERTOS_ENABLED

// ============================================================================
// STM32 HAL
// ============================================================================
#ifdef STM32_HAL

#include "stm32_hal.h"

namespace litepb {

uint32_t get_current_time_ms()
{
    return HAL_GetTick();
}

} // namespace litepb

#endif // STM32_HAL

// ============================================================================
// ZEPHYR RTOS
// ============================================================================
#ifdef CONFIG_ZEPHYR_KERNEL

#include <zephyr/kernel.h>

namespace litepb {

uint32_t get_current_time_ms()
{
    return k_uptime_get_32();
}

} // namespace litepb

#endif // CONFIG_ZEPHYR_KERNEL

// ============================================================================
// MBED OS
// ============================================================================
#ifdef MBED_CONF_RTOS_PRESENT

#include "mbed.h"

namespace litepb {

uint32_t get_current_time_ms()
{
    return Kernel::get_ms_count();
}

} // namespace litepb

#endif // MBED_CONF_RTOS_PRESENT

// ============================================================================
// LINUX/POSIX with clock_gettime (more efficient than std::chrono)
// ============================================================================
#ifdef USE_POSIX_CLOCK

#include <time.h>

namespace litepb {

uint32_t get_current_time_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint32_t>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

} // namespace litepb

#endif // USE_POSIX_CLOCK

// ============================================================================
// CUSTOM IMPLEMENTATION TEMPLATE
// ============================================================================
#if 0
// Example: Custom platform implementation
// Copy this template to your application code and define YOUR_PLATFORM

#ifdef YOUR_PLATFORM

namespace litepb {

uint32_t get_current_time_ms()
{
    // Your platform-specific time implementation here
    // Must return monotonic milliseconds for proper timeout handling
    return your_platform_get_ticks();
}

} // namespace litepb

#endif // YOUR_PLATFORM
#endif
