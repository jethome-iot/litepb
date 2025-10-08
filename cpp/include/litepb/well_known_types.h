/**
 * @file well_known_types.h
 * @brief Google Protocol Buffers Well-Known Types support for LitePB
 *
 * This header provides C++ implementations for Google's well-known types,
 * including Timestamp, Duration, Any, Empty, and wrapper types.
 * These types are wire-format compatible with the standard protobuf definitions.
 *
 * @copyright Copyright (c) 2025
 * @license MIT License
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include "litepb/core/unknown_fields.h"

namespace google {
namespace protobuf {

/**
 * @brief Represents an empty message
 * Compatible with google.protobuf.Empty
 */
struct Empty {
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
};

/**
 * @brief Represents a point in time
 * Compatible with google.protobuf.Timestamp
 * 
 * A timestamp represents an absolute point in time independent of any time zone
 * or calendar, represented as seconds and fractions of seconds at nanosecond
 * resolution in UTC Epoch time.
 */
struct Timestamp {
    int64_t seconds = 0;  // Seconds since Unix epoch (Jan 1, 1970 00:00:00 UTC)
    int32_t nanos = 0;    // Non-negative fractions of a second at nanosecond resolution
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Utility methods
    
    /**
     * @brief Convert to std::chrono::system_clock::time_point
     */
    std::chrono::system_clock::time_point to_time_point() const {
        using namespace std::chrono;
        auto duration = seconds * seconds{1} + nanoseconds{nanos};
        return system_clock::time_point{duration_cast<system_clock::duration>(duration)};
    }
    
    /**
     * @brief Create from std::chrono::system_clock::time_point
     */
    static Timestamp from_time_point(const std::chrono::system_clock::time_point& tp) {
        using namespace std::chrono;
        auto duration = tp.time_since_epoch();
        auto secs = duration_cast<std::chrono::seconds>(duration);
        auto ns = duration_cast<std::chrono::nanoseconds>(duration - secs);
        
        Timestamp ts;
        ts.seconds = secs.count();
        ts.nanos = static_cast<int32_t>(ns.count());
        return ts;
    }
    
    /**
     * @brief Get current time as Timestamp
     */
    static Timestamp now() {
        return from_time_point(std::chrono::system_clock::now());
    }
    
    /**
     * @brief Convert to Unix timestamp (seconds since epoch)
     */
    int64_t to_unix_seconds() const {
        return seconds;
    }
    
    /**
     * @brief Create from Unix timestamp
     */
    static Timestamp from_unix_seconds(int64_t unix_seconds) {
        Timestamp ts;
        ts.seconds = unix_seconds;
        ts.nanos = 0;
        return ts;
    }
};

/**
 * @brief Represents a time duration
 * Compatible with google.protobuf.Duration
 * 
 * A Duration represents a signed, fixed-length span of time represented
 * as a count of seconds and fractions of seconds at nanosecond resolution.
 */
struct Duration {
    int64_t seconds = 0;  // Signed seconds of the duration
    int32_t nanos = 0;    // Signed fractions of a second at nanosecond resolution
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Utility methods
    
    /**
     * @brief Convert to std::chrono::duration
     */
    template<typename Rep = int64_t, typename Period = std::nano>
    std::chrono::duration<Rep, Period> to_chrono_duration() const {
        using namespace std::chrono;
        auto total_ns = seconds * 1'000'000'000LL + nanos;
        return duration<Rep, Period>{duration_cast<duration<Rep, Period>>(nanoseconds{total_ns})};
    }
    
    /**
     * @brief Create from std::chrono::duration
     */
    template<typename Rep, typename Period>
    static Duration from_chrono_duration(const std::chrono::duration<Rep, Period>& d) {
        using namespace std::chrono;
        auto ns = duration_cast<nanoseconds>(d);
        
        Duration dur;
        dur.seconds = ns.count() / 1'000'000'000LL;
        dur.nanos = static_cast<int32_t>(ns.count() % 1'000'000'000LL);
        return dur;
    }
    
    /**
     * @brief Get total milliseconds
     */
    int64_t to_millis() const {
        return seconds * 1000 + nanos / 1'000'000;
    }
    
    /**
     * @brief Create from milliseconds
     */
    static Duration from_millis(int64_t millis) {
        Duration d;
        d.seconds = millis / 1000;
        d.nanos = static_cast<int32_t>((millis % 1000) * 1'000'000);
        return d;
    }
};

/**
 * @brief Wrapper for string values
 * Compatible with google.protobuf.StringValue
 */
struct StringValue {
    std::string value;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    StringValue() = default;
    explicit StringValue(const std::string& v) : value(v) {}
    explicit StringValue(std::string&& v) : value(std::move(v)) {}
    
    // Conversion operators
    operator const std::string&() const { return value; }
    operator std::string&() { return value; }
};

/**
 * @brief Wrapper for int32 values
 * Compatible with google.protobuf.Int32Value
 */
struct Int32Value {
    int32_t value = 0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    Int32Value() = default;
    explicit Int32Value(int32_t v) : value(v) {}
    
    // Conversion operators
    operator int32_t() const { return value; }
};

/**
 * @brief Wrapper for int64 values
 * Compatible with google.protobuf.Int64Value
 */
struct Int64Value {
    int64_t value = 0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    Int64Value() = default;
    explicit Int64Value(int64_t v) : value(v) {}
    
    // Conversion operators
    operator int64_t() const { return value; }
};

/**
 * @brief Wrapper for uint32 values
 * Compatible with google.protobuf.UInt32Value
 */
struct UInt32Value {
    uint32_t value = 0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    UInt32Value() = default;
    explicit UInt32Value(uint32_t v) : value(v) {}
    
    // Conversion operators
    operator uint32_t() const { return value; }
};

/**
 * @brief Wrapper for uint64 values
 * Compatible with google.protobuf.UInt64Value
 */
struct UInt64Value {
    uint64_t value = 0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    UInt64Value() = default;
    explicit UInt64Value(uint64_t v) : value(v) {}
    
    // Conversion operators
    operator uint64_t() const { return value; }
};

/**
 * @brief Wrapper for float values
 * Compatible with google.protobuf.FloatValue
 */
struct FloatValue {
    float value = 0.0f;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    FloatValue() = default;
    explicit FloatValue(float v) : value(v) {}
    
    // Conversion operators
    operator float() const { return value; }
};

/**
 * @brief Wrapper for double values
 * Compatible with google.protobuf.DoubleValue
 */
struct DoubleValue {
    double value = 0.0;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    DoubleValue() = default;
    explicit DoubleValue(double v) : value(v) {}
    
    // Conversion operators
    operator double() const { return value; }
};

/**
 * @brief Wrapper for bool values
 * Compatible with google.protobuf.BoolValue
 */
struct BoolValue {
    bool value = false;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    BoolValue() = default;
    explicit BoolValue(bool v) : value(v) {}
    
    // Conversion operators
    operator bool() const { return value; }
};

/**
 * @brief Wrapper for bytes values
 * Compatible with google.protobuf.BytesValue
 */
struct BytesValue {
    std::vector<uint8_t> value;
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Convenience constructors
    BytesValue() = default;
    explicit BytesValue(const std::vector<uint8_t>& v) : value(v) {}
    explicit BytesValue(std::vector<uint8_t>&& v) : value(std::move(v)) {}
    
    // Conversion operators
    operator const std::vector<uint8_t>&() const { return value; }
    operator std::vector<uint8_t>&() { return value; }
};

/**
 * @brief Represents any arbitrary protobuf message
 * Compatible with google.protobuf.Any
 * 
 * Any contains an arbitrary serialized protocol buffer message along with a
 * URL that describes the type of the serialized message.
 */
struct Any {
    std::string type_url;           // Type URL identifying the message type
    std::vector<uint8_t> value;      // Serialized message bytes
    
    // Unknown field preservation for forward/backward compatibility
    litepb::UnknownFieldSet unknown_fields;
    
    // Utility methods
    
    /**
     * @brief Check if this Any contains a message of the given type
     * @param full_type_name The full type name (e.g., "google.protobuf.Timestamp")
     */
    bool is(const std::string& full_type_name) const {
        // Type URLs typically have the format: type.googleapis.com/full.type.name
        // or just the full type name
        if (type_url.empty()) return false;
        
        // Check if type_url ends with the full_type_name
        if (type_url.size() >= full_type_name.size()) {
            return type_url.substr(type_url.size() - full_type_name.size()) == full_type_name;
        }
        return false;
    }
    
    /**
     * @brief Set the type URL for a given type name
     * @param full_type_name The full type name (e.g., "google.protobuf.Timestamp")
     * @param url_prefix The URL prefix (default: "type.googleapis.com/")
     */
    void set_type(const std::string& full_type_name, 
                  const std::string& url_prefix = "type.googleapis.com/") {
        type_url = url_prefix + full_type_name;
    }
};

} // namespace protobuf
} // namespace google