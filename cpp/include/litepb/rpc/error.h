/**
 * @file error.h
 * @brief RPC error handling and result types
 * 
 * This header defines the error handling system for LitePB RPC operations.
 * It provides error codes for protocol-level issues and a Result type for
 * combining return values with error status.
 * 
 * @copyright Copyright (c) 2024 JetHome LLC
 * @license MIT License
 */

#pragma once

#ifdef LITEPB_WITH_RPC

namespace litepb {

/**
 * @brief RPC error information structure
 * 
 * Contains error codes for RPC protocol-level errors. These represent
 * issues with the RPC framework itself, not application-level errors.
 * Application errors should be encoded in the response payload.
 */
struct RpcError
{
    /**
     * @brief RPC error code enumeration
     * 
     * These codes represent protocol and transport level errors detected
     * by the RPC framework. They do not include application-specific errors.
     */
    enum Code
    {
        OK                = 0,  ///< Operation completed successfully
        TIMEOUT           = 1,  ///< Request exceeded deadline before response
        PARSE_ERROR       = 2,  ///< Failed to parse message (malformed protobuf)
        TRANSPORT_ERROR   = 3,  ///< Transport layer failure (connection lost, etc.)
        HANDLER_NOT_FOUND = 4,  ///< No handler registered for method
        UNKNOWN           = 5   ///< Unknown error occurred
    };

    Code code;  ///< The error code

    /**
     * @brief Check if the operation succeeded
     * @return true if error code is OK, false otherwise
     */
    bool ok() const { return code == OK; }
};

/**
 * @brief Convert an RPC error code to a human-readable string
 * 
 * @param code The error code to convert
 * @return String description of the error
 * 
 * @example
 * @code{.cpp}
 * RpcError error;
 * error.code = RpcError::TIMEOUT;
 * std::cout << "Error: " << rpc_error_to_string(error.code) << std::endl;
 * // Output: "Error: RPC timeout"
 * @endcode
 */
inline const char* rpc_error_to_string(RpcError::Code code)
{
    switch (code) {
    case RpcError::OK:
        return "OK";
    case RpcError::TIMEOUT:
        return "RPC timeout";
    case RpcError::PARSE_ERROR:
        return "Parse error";
    case RpcError::TRANSPORT_ERROR:
        return "Transport error";
    case RpcError::HANDLER_NOT_FOUND:
        return "Handler not found";
    case RpcError::UNKNOWN:
        return "Unknown error";
    default:
        return "Unknown error";
    }
}

/**
 * @brief Result type combining a value with error status
 * 
 * Result<T> is used as the return type for RPC methods. It combines
 * the response value with RPC-level error information, allowing handlers
 * to indicate both success/failure and provide response data.
 * 
 * @tparam T The type of the successful result value
 * 
 * @example Server handler returning a result
 * @code{.cpp}
 * litepb::Result<Response> MyService::ProcessRequest(const Request& req) {
 *     litepb::Result<Response> result;
 *     
 *     if (req.id < 0) {
 *         // RPC succeeds but application returns error
 *         result.error.code = litepb::RpcError::OK;
 *         result.value.status = Response::INVALID_ID;
 *         return result;
 *     }
 *     
 *     // Populate successful response
 *     result.error.code = litepb::RpcError::OK;
 *     result.value.data = process_data(req.id);
 *     return result;
 * }
 * @endcode
 * 
 * @example Client checking result
 * @code{.cpp}
 * client.CallMethod(request, [](const litepb::Result<Response>& result) {
 *     if (!result.ok()) {
 *         // RPC-level error
 *         std::cerr << "RPC failed: " << rpc_error_to_string(result.error.code);
 *         return;
 *     }
 *     
 *     // Check application-level status
 *     if (result.value.status != Response::OK) {
 *         std::cerr << "Application error: " << result.value.status;
 *         return;
 *     }
 *     
 *     // Use successful result
 *     process_response(result.value);
 * });
 * @endcode
 */
template <typename T>
struct Result
{
    T value;         ///< The result value (only valid if error.ok() is true)
    RpcError error;  ///< RPC-level error information

    /**
     * @brief Check if the operation succeeded
     * @return true if error code is OK, false otherwise
     */
    bool ok() const { return error.ok(); }
};

} // namespace litepb

#endif // LITEPB_WITH_RPC
