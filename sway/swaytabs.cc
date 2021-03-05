// Shell version (on my laptop): 50ms
// No shell, but still exec of jq and swaymsg: 45ms.
// No jq, still shelling out to swaymsg both for read and write: ~0.000s user, 0.007s real.

#include<simdjson.h>

#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const char *swaymsg = "/usr/bin/swaymsg";
const char *jq = "/usr/bin/jq";

[[noreturn]] void run_swaymsg(int out)
{
  if (-1 == dup2(out, STDOUT_FILENO)) {
    std::cerr << "Failed to dup2 for swaymsg: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  execl(swaymsg, swaymsg, "-t", "get_tree", NULL);
  std::cerr << "Failed to run swaymsg: " << strerror(errno) << std::endl;
  exit(EXIT_FAILURE);
}

std::pair<bool,int> recurse(const simdjson::dom::element&elem, std::vector<char>&stack)
{
  if (static_cast<bool>(elem["focused"])) {
    int parents = 0;
    for (int i = stack.size()-1; i>=0; i--) {
      parents++;
      if (stack[i]) {
	return std::make_pair(true,parents);
      }
    }
    return std::make_pair(true,0);
  }
  for (const auto& e : simdjson::dom::array(elem["nodes"])) {
    stack.push_back(static_cast<std::string_view>(elem["layout"]) == "tabbed");
    auto res = recurse(e, stack);
    if (res.first) {
      return res;
    }
    stack.pop_back();
  }
  return std::make_pair(false,0);
}

int parent_simdjson()
{
  int swaypipe[2];
  if (-1 == pipe(swaypipe)) {
    throw std::runtime_error(std::string("pipe(): ") + strerror(errno));
  }
  
  const auto swaypid = fork();
  if (swaypid == -1) {
    // leaks fds, but we're exiting anyway.
    throw std::runtime_error(std::string("fork(): ") + strerror(errno));
  }

  if (swaypid == 0) {
    run_swaymsg(swaypipe[1]);
  }
  close(swaypipe[1]);

  int fd = swaypipe[0];
  std::vector<char> jsons;
  for (;;) {
    std::array<char, 4096> buf;
    const auto rc = read(fd, buf.data(), buf.size());
    if (rc == -1) {
      // leaks fds, but we're exiting anyway.
      throw std::runtime_error(std::string("read(): ") + strerror(errno));
    }
    if (rc == 0) {
      break;
    }
    std::copy(buf.data(), buf.data()+rc, std::back_inserter(jsons));
  }
  close(fd);
  simdjson::dom::parser parser;
  simdjson::dom::element json = parser.parse(jsons.data(), jsons.size());
  std::vector<char> stack;
  stack.reserve(64);
  const auto ret = recurse(json, stack).second;

  // Check error code after parsing, to give swaymsg time to exit.
  // Micro-optimization.
  int status;
  if (-1 == waitpid(swaypid, &status, 0)) {
    throw std::runtime_error(std::string("waitpid(): ") + strerror(errno));
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status)) {
    throw std::runtime_error("swaymsg did not exit cleanly");
  }
  return ret;
}

int main(int argc, char** argv)
{
  std::string cmds;
  cmds.reserve(1000);
  if (argc > 2) {
    std::cerr << "Extra args on command line. Only one arg for Sway commands allowed\n";
    exit(EXIT_FAILURE);
  }

  for (int i = parent_simdjson(); i; i--) {
    cmds += "focus parent,";
  }

  if (argc == 2) {
    cmds += argv[1];
  }
  execl(swaymsg,swaymsg,cmds.c_str(), nullptr);
  std::cerr << "Failed to exec <" << swaymsg << ">: " << strerror(errno) << std::endl;
  exit(EXIT_FAILURE);
}
