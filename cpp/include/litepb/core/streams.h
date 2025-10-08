/**
 * @file streams.h
 * @brief Stream interfaces for LitePB serialization
 *
 * This header defines the abstract stream interfaces used throughout LitePB
 * for reading and writing Protocol Buffers data. It also provides concrete
 * implementations for common use cases like memory buffers and fixed-size arrays.
 *
 * The stream abstraction allows LitePB to work with various data sources and
 * destinations including memory, files, network sockets, or custom hardware interfaces.
 *
 * @copyright Copyright (c) 2025 JetHome LLC
 * @license MIT License
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace litepb {

/**
 * @brief Abstract output stream interface for writing serialized data
 *
 * OutputStream provides a uniform interface for writing bytes to various destinations.
 * Implementations can write to memory buffers, files, network sockets, or any other
 * byte-oriented output device.
 *
 * @note All write operations should be atomic - either all bytes are written or none
 */
class OutputStream {
public:
    virtual ~OutputStream() = default;

    /**
     * @brief Write bytes to the stream
     *
     * @param data Pointer to the data to write
     * @param size Number of bytes to write
     * @return true if all bytes were written successfully, false on error
     */
    virtual bool write(const uint8_t * data, size_t size) = 0;

    /**
     * @brief Get the current write position in the stream
     *
     * @return Number of bytes written to the stream so far
     */
    virtual size_t position() const = 0;
};

/**
 * @brief Abstract input stream interface for reading serialized data
 *
 * InputStream provides a uniform interface for reading bytes from various sources.
 * Implementations can read from memory buffers, files, network sockets, or any other
 * byte-oriented input device.
 *
 * @note All read operations should be atomic - either all requested bytes are read or none
 */
class InputStream {
public:
    virtual ~InputStream() = default;

    /**
     * @brief Read bytes from the stream
     *
     * @param data Buffer to store the read data
     * @param size Number of bytes to read
     * @return true if all bytes were read successfully, false on error or EOF
     */
    virtual bool read(uint8_t * data, size_t size) = 0;

    /**
     * @brief Skip bytes in the stream without reading them
     *
     * @param size Number of bytes to skip
     * @return true if skip succeeded, false on error or EOF
     */
    virtual bool skip(size_t size) = 0;

    /**
     * @brief Get the current read position in the stream
     *
     * @return Number of bytes read from the stream so far
     */
    virtual size_t position() const = 0;

    /**
     * @brief Get the number of bytes available for reading
     *
     * @return Number of bytes that can be read without blocking
     */
    virtual size_t available() const = 0;
};

/**
 * @brief Dynamic memory buffer output stream
 *
 * BufferOutputStream uses a std::vector to provide a dynamically growing
 * output buffer. This is the most commonly used output stream for serialization
 * when the message size is not known in advance.
 *
 * @example
 * @code{.cpp}
 * litepb::BufferOutputStream output;
 * MyMessage msg;
 * msg.id = 123;
 *
 * if (litepb::serialize(msg, output)) {
 *     // Access the serialized data
 *     send_data(output.data(), output.size());
 * }
 * @endcode
 */
class BufferOutputStream : public OutputStream {
    std::vector<uint8_t> buffer_;

public:
    /**
     * @brief Default constructor creates an empty output buffer
     */
    BufferOutputStream() = default;

    /**
     * @brief Write bytes to the buffer
     *
     * Appends the given bytes to the internal buffer, growing it as needed.
     *
     * @param data Pointer to the data to write
     * @param size Number of bytes to write
     * @return Always returns true (dynamic buffer cannot fail)
     */
    bool write(const uint8_t * data, size_t size) override
    {
        if (size == 0)
            return true;
        if (data) {
            buffer_.insert(buffer_.end(), data, data + size);
        } else {
            buffer_.insert(buffer_.end(), size, 0);
        }
        return true;
    }

    /**
     * @brief Get the current write position
     * @return Number of bytes written to the buffer
     */
    size_t position() const override { return buffer_.size(); }

    /**
     * @brief Get a pointer to the buffer data
     * @return Pointer to the internal buffer data
     */
    const uint8_t * data() const { return buffer_.data(); }

    /**
     * @brief Get the size of the buffer
     * @return Number of bytes in the buffer
     */
    size_t size() const { return buffer_.size(); }

    /**
     * @brief Clear the buffer contents
     */
    void clear() { buffer_.clear(); }
};

/**
 * @brief Memory buffer input stream
 *
 * BufferInputStream provides read access to a memory buffer. It does not
 * own the buffer memory, so the buffer must remain valid for the lifetime
 * of the stream.
 *
 * @example
 * @code{.cpp}
 * uint8_t data[] = {...};  // Serialized message bytes
 * litepb::BufferInputStream input(data, sizeof(data));
 *
 * MyMessage msg;
 * if (litepb::parse(msg, input)) {
 *     // Message successfully parsed
 * }
 * @endcode
 */
class BufferInputStream : public InputStream {
    const uint8_t * data_;
    size_t size_;
    size_t pos_;

public:
    /**
     * @brief Construct an input stream from a buffer
     *
     * @param data Pointer to the buffer data (not owned by stream)
     * @param size Size of the buffer in bytes
     */
    BufferInputStream(const uint8_t * data, size_t size)
        : data_(data)
        , size_(size)
        , pos_(0)
    {
    }

    /**
     * @brief Read bytes from the buffer
     *
     * @param data Buffer to store the read data
     * @param size Number of bytes to read
     * @return true if all bytes were read, false if insufficient data
     */
    bool read(uint8_t * data, size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        if (data && size > 0) {
            std::memcpy(data, data_ + pos_, size);
        }
        pos_ += size;
        return true;
    }

    /**
     * @brief Skip bytes in the buffer
     *
     * @param size Number of bytes to skip
     * @return true if skip succeeded, false if insufficient data
     */
    bool skip(size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        pos_ += size;
        return true;
    }

    /**
     * @brief Get the current read position
     * @return Number of bytes read from the buffer
     */
    size_t position() const override { return pos_; }

    /**
     * @brief Get the number of bytes available for reading
     * @return Number of unread bytes remaining
     */
    size_t available() const override { return size_ > pos_ ? size_ - pos_ : 0; }
};

/**
 * @brief Fixed-size output stream for embedded systems
 *
 * FixedOutputStream provides a compile-time fixed-size buffer for output.
 * This is ideal for embedded systems where dynamic memory allocation must
 * be avoided. The buffer is allocated on the stack.
 *
 * @tparam SIZE The size of the buffer in bytes
 *
 * @example
 * @code{.cpp}
 * // Create a 256-byte output buffer on the stack
 * litepb::FixedOutputStream<256> output;
 *
 * MyMessage msg;
 * if (litepb::serialize(msg, output)) {
 *     uart_send(output.data(), output.position());
 * }
 * @endcode
 */
template <size_t SIZE>
class FixedOutputStream : public OutputStream {
    uint8_t buffer_[SIZE];
    size_t pos_;

public:
    /**
     * @brief Default constructor initializes an empty buffer
     */
    FixedOutputStream()
        : pos_(0)
    {
    }

    /**
     * @brief Write bytes to the fixed buffer
     *
     * @param data Pointer to the data to write
     * @param size Number of bytes to write
     * @return true if write succeeded, false if buffer would overflow
     */
    bool write(const uint8_t * data, size_t size) override
    {
        if (pos_ + size > SIZE)
            return false;
        if (data && size > 0) {
            std::memcpy(buffer_ + pos_, data, size);
        }
        pos_ += size;
        return true;
    }

    /**
     * @brief Get the current write position
     * @return Number of bytes written to the buffer
     */
    size_t position() const override { return pos_; }

    /**
     * @brief Get a pointer to the buffer data
     * @return Pointer to the internal buffer
     */
    const uint8_t * data() const { return buffer_; }

    /**
     * @brief Get the capacity of the buffer
     * @return Total size of the buffer in bytes
     */
    size_t capacity() const { return SIZE; }

    /**
     * @brief Clear the buffer (reset write position to 0)
     */
    void clear() { pos_ = 0; }
};

/**
 * @brief Fixed-size input stream for embedded systems
 *
 * FixedInputStream provides a compile-time fixed-size buffer for input.
 * The data is copied into an internal buffer on construction, making it
 * safe to use even if the original data source is freed.
 *
 * @tparam SIZE The size of the internal buffer in bytes
 *
 * @example
 * @code{.cpp}
 * uint8_t received_data[128];
 * size_t received_size = uart_receive(received_data, sizeof(received_data));
 *
 * // Create input stream with internal copy of data
 * litepb::FixedInputStream<256> input(received_data, received_size);
 *
 * MyMessage msg;
 * if (litepb::parse(msg, input)) {
 *     // Message successfully parsed
 * }
 * @endcode
 */
template <size_t SIZE>
class FixedInputStream : public InputStream {
    uint8_t buffer_[SIZE];
    size_t size_;
    size_t pos_;

public:
    /**
     * @brief Construct an input stream with a copy of the data
     *
     * @param data Pointer to the data to copy
     * @param size Number of bytes to copy (truncated to SIZE if larger)
     */
    FixedInputStream(const uint8_t * data, size_t size)
        : size_(0)
        , pos_(0)
    {
        if (data && size > 0) {
            size_ = size > SIZE ? SIZE : size;
            std::memcpy(buffer_, data, size_);
        }
    }

    /**
     * @brief Read bytes from the buffer
     *
     * @param data Buffer to store the read data
     * @param size Number of bytes to read
     * @return true if all bytes were read, false if insufficient data
     */
    bool read(uint8_t * data, size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        if (data && size > 0) {
            std::memcpy(data, buffer_ + pos_, size);
        }
        pos_ += size;
        return true;
    }

    /**
     * @brief Skip bytes in the buffer
     *
     * @param size Number of bytes to skip
     * @return true if skip succeeded, false if insufficient data
     */
    bool skip(size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        pos_ += size;
        return true;
    }

    /**
     * @brief Get the current read position
     * @return Number of bytes read from the buffer
     */
    size_t position() const override { return pos_; }

    /**
     * @brief Get the number of bytes available for reading
     * @return Number of unread bytes remaining
     */
    size_t available() const override { return size_ > pos_ ? size_ - pos_ : 0; }
};

} // namespace litepb
