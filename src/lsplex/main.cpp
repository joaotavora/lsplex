#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>

#include <cxxopts.hpp>

int main(int argc, char* argv[]) {

  cxxopts::Options options(*argv, "A language server proxy");
  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
  ;
  // clang-format on
  auto result = options.parse(argc, argv);
  if (result["help"].as<bool>()) {
    fmt::println("{}", options.help());
    return 0;
  }

  if (result["version"].as<bool>()) {
    fmt::println("LsPlex, version {}", LSPLEX_VERSION);
    return 0;
  }

  #ifndef NDEBUG
  (void)setvbuf(stdout, nullptr, _IONBF, 0);
  #endif
  lsplex::LsPlex lsplex({lsplex::LsInvocation({"echo"})});
  lsplex.start();
}
