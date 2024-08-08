#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>

#include <cxxopts.hpp>

int main(int argc, char* argv[]) {
  cxxopts::Options options(*argv, "A language server proxy");
  // clang-format off
  options.positional_help("-- PROGRAM PROGRAM-ARGS...")
         .add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("program", "The primary LS program to run", cxxopts::value<std::string>())
    ("program-args", "The primary LS args", cxxopts::value<std::vector<std::string>>()->default_value({}));
  // clang-format on

  options.parse_positional({"program", "program-args"});
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
  fmt::println(stderr, "Starting lsplex...");

  auto program = result["program"].as<std::string>();
  auto args = result["program-args"].as<std::vector<std::string>>();
  args.erase(args.begin());  // because cxxopts reasons

  lsplex::LsPlex lsplex({lsplex::LsContact{program, args}});
  lsplex.start();
}
