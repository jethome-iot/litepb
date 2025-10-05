#ifndef LITEPB_RPC_ERROR_H
#define LITEPB_RPC_ERROR_H

namespace litepb {

struct RpcError
{
    enum Code
    {
        OK                = 0,
        TIMEOUT           = 1,
        PARSE_ERROR       = 2,
        TRANSPORT_ERROR   = 3,
        HANDLER_NOT_FOUND = 4,
        UNKNOWN           = 5
    };

    Code code;

    bool ok() const { return code == OK; }
};

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

template <typename T>
struct Result
{
    T value;
    RpcError error;

    bool ok() const { return error.ok(); }
};

} // namespace litepb

#endif