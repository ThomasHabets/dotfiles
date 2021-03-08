#include "swaylib.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>

#include <vector>

// Return current id and other tabs.
std::pair<uint64_t, std::vector<uint64_t>>
recurse(const simdjson::dom::element& elem,
        const std::vector<uint64_t>& parent_tabs,
        int parent_index = -1)
{
    if (static_cast<bool>(elem["focused"])) {
        // Found focused window.
        // std::cout << "Found focused " << parent_tabs.size() << "\n";
        // If not in tab, do nothing.
        if (parent_tabs.empty()) {
            return std::make_pair(-1, std::vector<uint64_t>{});
        }
        return std::make_pair(parent_index, parent_tabs);
    }
    std::vector<uint64_t> mytabs;
    if (static_cast<std::string_view>(elem["layout"]) == "tabbed") {
        for (const auto& e : simdjson::dom::array(elem["nodes"])) {
            mytabs.push_back(e["pid"]);
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
    return std::make_pair(false, std::vector<uint64_t>{});
}

int tab_pid(const Sway::Parsed& json, bool relative, int ofs)
{
    std::vector<uint64_t> tabs;
    auto vals = recurse(json.elem, tabs);
    if (false) {
        std::cout << "active pid: " << vals.first << "\npids (" << vals.second.size()
                  << ")\n";
        for (const auto& pid : vals.second) {
            std::cout << " " << pid << std::endl;
        }
    }
    if (vals.second.empty()) {
        return 0;
    }
    return vals
        .second[(vals.second.size() + relative * vals.first + ofs) % vals.second.size()];
}

int main(int argc, char** argv)
{
    struct option long_options[] = { { "prev", no_argument, 0, 0 }, { 0, 0, 0, 0 } };
    int c;
    bool relative = true;
    int ofs = 1;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                relative = true;
                ofs = -1;
            }
        }
    }
    std::string cmds;
    cmds.reserve(1000);
    if (argc > 2) {
        std::cerr
            << "Extra args on command line. Only one arg for Sway commands allowed\n";
        exit(EXIT_FAILURE);
    }

    Sway sway;
    sway.command(std::string("[pid=") +
                 std::to_string(tab_pid(sway.get_tree(), relative, ofs)) + "] focus");
}
