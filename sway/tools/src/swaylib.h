// -*- c++ -*-
// A faster version of std::vector, because it doesn't do init.
// Main gain is that since it (unlike std::vector) isuncopyable, we know that we
// won't copy.
#include <simdjson.h>

#include <cstdint>

template <typename T>
class Buffer
{
public:
    static_assert(std::is_trivial<T>(), "Buffer only handles trivial types");

    Buffer(size_t size) : buf_(new T[size]), size_(size) {}
    Buffer(Buffer&& rhs) noexcept
        : buf_(std::exchange(rhs.buf_, nullptr)), size_(rhs.size_)
    {
    }

    // No copy.
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer& operator=(Buffer&& rhs) noexcept = delete;
    /*  Buffer&operator=(Buffer&&rhs) noexcept
    {
      delete[] buf_;
      buf_ = std::exchange(rhs.buf_, nullptr);
      size_ = rhs.size_;
      return *this;
      }*/
    ~Buffer() { delete[] buf_; }
    T* data() const noexcept { return buf_; }
    const T* begin() const noexcept { return buf_; }
    const T* end() const noexcept { return buf_ + size_; }
    size_t size() const noexcept { return size_; }

private:
    T* buf_;
    const size_t size_;
};

class Sway
{
public:
    Sway();
    Sway(const Sway&) = delete;
    Sway(Sway&&) = delete;
    Sway& operator=(const Sway&) = delete;
    Sway& operator=(Sway&&) = delete;
    ~Sway();

    struct Parsed {
        simdjson::dom::element elem;
        // Pointer to vector since elem points into the data.
        // Performance: shave off microseconds by using manual memory management?
        std::unique_ptr<Buffer<char>> data;
    };

    Parsed get_tree();
    void command(const std::string&);

private:
    std::pair<uint32_t, Buffer<char>> read_packet();
    simdjson::dom::parser parser_;
    int fd_;
};
