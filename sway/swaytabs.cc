// Shell version (on my laptop): 50ms. 202M instructions.
// No shell, but still exec of jq and swaymsg: 45ms.
// No jq, still shelling out to swaymsg both for read and write: ~0.000s user, 0.007s
// real.
//
// All in-house: ~0.000s user, 0.002s real. 4M instructions (2M user)
//

#include <simdjson.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <array>
#include <iostream>
#include <string>
#include <vector>

const char* swaymsg = "/usr/bin/swaymsg";
const char* jq = "/usr/bin/jq";

// Sway IPC types.
constexpr int RUN_COMMAND = 0;
constexpr int GET_TREE = 4;

struct __attribute__((packed)) sway_header_t {
    char magic[6] = { 'i', '3', '-', 'i', 'p', 'c' };
    uint32_t length;
    uint32_t type;
};

// A faster version of std::vector, because it doesn't do init.
// Main gain is that since it (unlike std::vector) isuncopyable, we know that we won't
// copy.
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

Sway::Sway() : fd_(socket(PF_UNIX, SOCK_STREAM, 0))
{
    if (fd_ == -1) {
        throw std::runtime_error(std::string("failed create UNIX socket: ") +
                                 strerror(errno));
    }
    struct sockaddr_un sa {
    };
    sa.sun_family = AF_UNIX;
    const auto fn = getenv("SWAYSOCK");
    if (!fn) {
        throw std::runtime_error("SWAYSOCK not set");
    }
    strncpy(sa.sun_path, fn, sizeof(sa.sun_path));

    if (connect(fd_, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa))) {
        auto err = errno;
        close(fd_);
        throw std::runtime_error(std::string("failed to connect() to sway: ") +
                                 strerror(err));
    }
}

void Sway::command(const std::string& cmds)
{
    sway_header_t head;
    head.type = RUN_COMMAND;
    head.length = cmds.size();
    struct iovec iov[2];
    iov[0].iov_base = static_cast<void*>(&head);
    iov[1].iov_base = (void*)cmds.data();

    size_t written = 0;
    const size_t total = sizeof(head) + cmds.size();
    do {
        iov[0].iov_len = std::max(size_t(0), sizeof(head) - written);
        iov[1].iov_len = written <= sizeof(head) ? cmds.size()
                                                 : (cmds.size() + sizeof(head) - written);
        const auto rc = writev(fd_, iov, 2);
        if (rc == -1) {
            throw std::runtime_error(std::string("writev() to sway: ") + strerror(errno));
        }
        written += rc;
    } while (written != total);

    // Check for success.
    //
    // Parsing is overkill here. Just look for anything that isn't success.
    const auto reply = read_packet();
    const char e[] = "false"; // "success": false
    if (memmem(reply.second.data(), reply.second.size(), e, sizeof e)) {
        throw std::runtime_error(std::string("commands failed: ") +
                                 std::string(reply.second.begin(), reply.second.end()));
    }
}


Sway::~Sway()
{
    if (fd_ != -1) {
        close(fd_);
    }
}

std::pair<uint32_t, Buffer<char>> Sway::read_packet()
{
    // Read header.
    sway_header_t header;
    {
        const auto rc = read(fd_, reinterpret_cast<void*>(&header), sizeof(header));
        if (rc != sizeof(header)) {
            throw std::runtime_error(std::string("read(header): ") + strerror(errno));
        }
    }
    Buffer<char> buf(header.length);

    size_t n = header.length;
    for (char* p = buf.data(); n;) {
        const auto rc = read(fd_, p, n);
        if (rc == -1) {
            throw std::runtime_error(std::string("read(data): ") + strerror(errno));
        }
        n -= rc;
        p += rc;
    }
    return std::make_pair((uint32_t)header.type, std::move(buf));
}


Sway::Parsed Sway::get_tree()
{
    sway_header_t req;
    req.length = 0;
    req.type = GET_TREE;
    const auto rc = write(fd_, reinterpret_cast<void*>(&req), sizeof(req));

    auto reply = read_packet();
    Parsed ret;
    ret.data = std::make_unique<Buffer<char>>(std::move(reply.second));
    ret.elem = parser_.parse(ret.data->data(), ret.data->size());
    return ret;
}

std::pair<bool, int> recurse(const simdjson::dom::element& elem, std::vector<char>& stack)
{
    if (static_cast<bool>(elem["focused"])) {
        int parents = 0;
        for (int i = stack.size() - 1; i >= 0; i--) {
            parents++;
            if (stack[i]) {
                return std::make_pair(true, parents);
            }
        }
        return std::make_pair(true, 0);
    }
    for (const auto& e : simdjson::dom::array(elem["nodes"])) {
        stack.push_back(static_cast<std::string_view>(elem["layout"]) == "tabbed");
        auto res = recurse(e, stack);
        if (res.first) {
            return res;
        }
        stack.pop_back();
    }
    return std::make_pair(false, 0);
}

int parent_simdjson(const Sway::Parsed& json)
{
    std::vector<char> stack;
    stack.reserve(64);
    return recurse(json.elem, stack).second;
}

int main(int argc, char** argv)
{
    std::string cmds;
    cmds.reserve(1000);
    if (argc > 2) {
        std::cerr
            << "Extra args on command line. Only one arg for Sway commands allowed\n";
        exit(EXIT_FAILURE);
    }
    Sway sway;

    for (int i = parent_simdjson(sway.get_tree()); i; i--) {
        cmds += "focus parent,";
    }

    if (argc == 2) {
        cmds += argv[1];
    }
    sway.command(cmds);
}
