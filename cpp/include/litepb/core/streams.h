#ifndef LITEPB_CORE_STREAMS_H
#define LITEPB_CORE_STREAMS_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace litepb {

class OutputStream
{
public:
    virtual ~OutputStream()                              = default;
    virtual bool write(const uint8_t* data, size_t size) = 0;
    virtual size_t position() const                      = 0;
};

class InputStream
{
public:
    virtual ~InputStream()                        = default;
    virtual bool read(uint8_t* data, size_t size) = 0;
    virtual bool skip(size_t size)                = 0;
    virtual size_t position() const               = 0;
    virtual size_t available() const              = 0;
};

class BufferOutputStream : public OutputStream
{
    std::vector<uint8_t> buffer_;

public:
    BufferOutputStream() = default;

    bool write(const uint8_t* data, size_t size) override
    {
        if (size == 0)
            return true;
        if (data) {
            buffer_.insert(buffer_.end(), data, data + size);
        }
        else {
            buffer_.insert(buffer_.end(), size, 0);
        }
        return true;
    }

    size_t position() const override { return buffer_.size(); }

    const uint8_t* data() const { return buffer_.data(); }

    size_t size() const { return buffer_.size(); }

    void clear() { buffer_.clear(); }
};

class BufferInputStream : public InputStream
{
    const uint8_t* data_;
    size_t size_;
    size_t pos_;

public:
    BufferInputStream(const uint8_t* data, size_t size) : data_(data), size_(size), pos_(0) {}

    bool read(uint8_t* data, size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        if (data && size > 0) {
            std::memcpy(data, data_ + pos_, size);
        }
        pos_ += size;
        return true;
    }

    bool skip(size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        pos_ += size;
        return true;
    }

    size_t position() const override { return pos_; }

    size_t available() const override { return size_ > pos_ ? size_ - pos_ : 0; }
};

template <size_t SIZE>
class FixedOutputStream : public OutputStream
{
    uint8_t buffer_[SIZE];
    size_t pos_;

public:
    FixedOutputStream() : pos_(0) {}

    bool write(const uint8_t* data, size_t size) override
    {
        if (pos_ + size > SIZE)
            return false;
        if (data && size > 0) {
            std::memcpy(buffer_ + pos_, data, size);
        }
        pos_ += size;
        return true;
    }

    size_t position() const override { return pos_; }

    const uint8_t* data() const { return buffer_; }

    size_t capacity() const { return SIZE; }

    void clear() { pos_ = 0; }
};

template <size_t SIZE>
class FixedInputStream : public InputStream
{
    uint8_t buffer_[SIZE];
    size_t size_;
    size_t pos_;

public:
    FixedInputStream(const uint8_t* data, size_t size) : size_(0), pos_(0)
    {
        if (data && size > 0) {
            size_ = size > SIZE ? SIZE : size;
            std::memcpy(buffer_, data, size_);
        }
    }

    bool read(uint8_t* data, size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        if (data && size > 0) {
            std::memcpy(data, buffer_ + pos_, size);
        }
        pos_ += size;
        return true;
    }

    bool skip(size_t size) override
    {
        if (pos_ + size > size_)
            return false;
        pos_ += size;
        return true;
    }

    size_t position() const override { return pos_; }

    size_t available() const override { return size_ > pos_ ? size_ - pos_ : 0; }
};

} // namespace litepb

#endif
