#include <getopt.h>

#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "swaylib.h"

namespace {
bool verbose = false;

// Return index of current tab, and ids of all tabs.
std::pair<int, std::vector<uint64_t>> recurse(const simdjson::dom::element& elem,
                                              const std::vector<uint64_t>& parent_tabs,
                                              int parent_index = -1)
{
    if (static_cast<bool>(elem["focused"])) {
        // Found focused window.
        // std::cout << "Found focused " << parent_tabs.size() << "\n";
        // If not in tab, do nothing.
        if (parent_tabs.empty()) {
            return std::make_pair(0, std::vector<uint64_t>{});
        }
        return std::make_pair(parent_index, parent_tabs);
    }
    std::vector<uint64_t> mytabs;
    if (static_cast<std::string_view>(elem["layout"]) == "tabbed") {
        for (const auto& e : simdjson::dom::array(elem["nodes"])) {
            // Find the first leaf node under this tab.
            //
            // This assumes that any node in the graph that doesn't have a id is
            // a container that has at least one child node.
            std::function<uint64_t(const simdjson::dom::element& elem)> f = [&](
                const auto& elem) -> auto
            {
                uint64_t v;
                if (elem["id"].get(v)) {
                    return f(simdjson::dom::array(elem["nodes"]).at(0));
                }
                return v;
            };
            mytabs.push_back(f(e));
        }
    }
    bool count_index = false;
    const auto& tabs = [&]() -> const std::vector<uint64_t>& {
        if (mytabs.empty()) {
            return parent_tabs;
        }
        count_index = true;
        return mytabs;
    }();
    if (count_index) {
        parent_index = 0;
    }
    for (const auto& e : simdjson::dom::array(elem["nodes"])) {
        auto res = recurse(e, tabs, parent_index++);
        if (!res.second.empty()) {
            return res;
        }
    }
    return std::make_pair(0, std::vector<uint64_t>{});
}

// Return the tab id to change to, or 0 for "no change".
uint64_t tab_id(const Sway::Parsed& json, bool relative, int ofs)
{
    std::vector<uint64_t> tabs;
    auto vals = recurse(json.elem, tabs);
    if (verbose) {
        std::cout << "active id: " << vals.first << "\n"
                  << " requested relative=" << relative << " ofs=" << ofs << "\n"
                  << "ids (" << vals.second.size() << ")\n";
        for (const auto& id : vals.second) {
            std::cout << " " << id << std::endl;
        }
    }
    if (vals.second.empty()) {
        return 0;
    }
    return vals
        .second[(vals.second.size() + relative * vals.first + ofs) % vals.second.size()];
}
} // namespace

void usage(const char* argv0, int err)
{
    auto& o = [err]() -> std::ostream& {
        if (err) {
            return std::cerr;
        }
        return std::cout;
    }();
    o << "Usage: " << argv0 << " [options]\n"
      << "Options:\n"
      << "  -a, --absolute  Jump to absolute tab number\n"
      << "  -h, --help      Show this help text\n"
      << "  -p, --prev      Jump backwards\n"
      << "  -v, --verbose   Show verbose output\n";
    exit(err);
}

int main(int argc, char** argv)
{
    struct option long_options[] = {
        { "absolute", required_argument, 0, 'a' },
        { "help", no_argument, 0, 'h' },
        { "prev", no_argument, 0, 'p' },
        { "verbose", no_argument, 0, 'v' },
        { 0, 0, 0, 0 },
    };
    int c;
    bool relative = true;
    int ofs = 1;
    while ((c = getopt_long(argc, argv, "ahpv", long_options, nullptr)) != -1) {
        switch (c) {
        case 'a':
            relative = false;
            ofs = atoi(optarg);
            break;
        case 'p':
            relative = true;
            ofs = -1;
            break;
        case 'v':
            verbose = true;
            break;
        case 'h':
            usage(argv[0], 0);
        default:
            std::cerr << "Unrecognized argument(s)\n";
            usage(argv[0], EXIT_FAILURE);
        }
    }
    std::string cmds;
    cmds.reserve(1000);
    if (argc > 2) {
        std::cerr << "Extra args on command line. Only one arg for Sway commands "
                     "allowed\n";
        exit(EXIT_FAILURE);
    }

    Sway sway;
    const auto tab = tab_id(sway.get_tree(), relative, ofs);
    if (tab > 0) {
        sway.command("[con_id=" + std::to_string(tab) + "] focus");
    }
}
