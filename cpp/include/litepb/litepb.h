#ifndef LITEPB_H
#define LITEPB_H

#include "litepb/core/proto_reader.h"
#include "litepb/core/proto_writer.h"
#include "litepb/core/streams.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace litepb {

template <typename T>
class Serializer
{
public:
    static bool serialize(const T& msg, OutputStream& stream);
    static bool parse(T& msg, InputStream& stream);
    static size_t byte_size(const T& msg);
};

template <typename T>
inline bool serialize(const T& msg, OutputStream& stream)
{
    return Serializer<T>::serialize(msg, stream);
}

template <typename T>
inline bool parse(T& msg, InputStream& stream)
{
    return Serializer<T>::parse(msg, stream);
}

template <typename T>
inline size_t byte_size(const T& msg)
{
    return Serializer<T>::byte_size(msg);
}

} // namespace litepb

#endif
