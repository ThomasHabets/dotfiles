// Shell version (on my laptop): 50ms. 202M instructions.
// No shell, but still exec of jq and swaymsg: 45ms.
// No jq, still shelling out to swaymsg both for read and write: ~0.000s user,
// 0.007s real.
//
// All in-house: ~0.000s user, 0.002s real. 4M instructions (2M user)
//

#include <getopt.h>
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

#include "swaylib.h"

namespace {
// Return pair of:
// * true if in a tabbed container
// * number of levels to go up to focus on the tabbing container (0 if not in tabbing
//   container)
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

// return number of levels a tabbing container is "up", or 0 if not in a tabbing container
int parent_simdjson(const Sway::Parsed& json)
{
    std::vector<char> stack;
    stack.reserve(64);
    return recurse(json.elem, stack).second;
}

void usage(const char* argv0, int err)
{
    auto& o = [err]() -> std::ostream& {
        if (err) {
            return std::cerr;
        }
        return std::cout;
    }();
    o << "Usage: " << argv0 << " [options] <sway command>\n"
      << "Options:\n"
      << "  -h, --help      Show this help text\n";
    exit(err);
}
} // namespace

int main(int argc, char** argv)
{
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 },
    };
    int c;

    while ((c = getopt_long(argc, argv, "h", long_options, nullptr)) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0], 0);
        }
    }

    if (argc > optind + 1) {
        std::cerr << "Extra args on command line. Only one arg for Sway commands "
                     "allowed\n";
        usage(argv[0], EXIT_FAILURE);
    }

    Sway sway;

    std::string cmds;
    cmds.reserve(1000);
    for (int i = parent_simdjson(sway.get_tree()); i; i--) {
        cmds += "focus parent,";
    }

    if (argc == 2) {
        cmds += argv[1];
    }
    sway.command(cmds);
}
