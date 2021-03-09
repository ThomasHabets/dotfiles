#include "swaylib.h"
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

// Sway IPC types.
constexpr int RUN_COMMAND = 0;
constexpr int GET_TREE = 4;

struct __attribute__((packed)) sway_header_t {
    char magic[6] = { 'i', '3', '-', 'i', 'p', 'c' };
    uint32_t length;
    uint32_t type;
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
    if (rc == -1) {
        throw std::runtime_error(std::string("write(): ") + strerror(errno));
    }

    auto reply = read_packet();
    Parsed ret;
    ret.data = std::make_unique<Buffer<char>>(std::move(reply.second));
    ret.elem = parser_.parse(ret.data->data(), ret.data->size());
    return ret;
}
