#include <doctest/doctest.h>
#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>
#include <jsonrpc/pal/pal.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

TEST_CASE("Lsplex version") {
  static_assert(std::string_view(LSPLEX_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(LSPLEX_VERSION) == std::string("1.0.0"));
}

TEST_CASE("Loop some well behaved messages fully through LsPlex") {

  std::stringstream buffer;
  {
    std::ifstream file{"resources/well_behaved_messages.txt", std::ios::binary};
    buffer << file.rdbuf();
  }
  lsplex::jsonrpc::pal::redirector r{"resources/well_behaved_messages.txt"};
  std::thread th{[]{
#if defined(_MSC_VER) || defined(__MINGW64__)
    lsplex::LsContact contact{"./wincat.exe", {}};
#else
    lsplex::LsContact contact{"cat", {}};
#endif
    lsplex::LsPlex plex{{contact}};
    plex.start();
    ::close(STDOUT_FILENO);
  }};
  th.join();
  auto slurped = r.slurp();
  CHECK(slurped.size() == buffer.str().size());
  CHECK(slurped == buffer.str());
}
