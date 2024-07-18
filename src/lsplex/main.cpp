#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>

#include <cxxopts.hpp>
#include <string>
#include <unordered_map>

auto main(int argc, char **argv) -> int {
  const std::unordered_map<std::string, lsplex::LanguageCode> languages{
      {"en", lsplex::LanguageCode::en},
      {"de", lsplex::LanguageCode::de},
      {"es", lsplex::LanguageCode::es},
      {"fr", lsplex::LanguageCode::fr},
  };

  cxxopts::Options options(*argv, "A program to welcome the world!");

  std::string language;
  std::string name;

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("n,name", "Name to greet", cxxopts::value(name)->default_value("World"))
    ("l,lang", "Language code to use", cxxopts::value(language)->default_value("en"))
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

  auto lang_it = languages.find(language);
  if (lang_it == languages.end()) {
    fmt::println("Unknown language code: {}", language);
    return 1;
  }

  lsplex::LsPlex lsplex(name);
  fmt::println("{}", lsplex.greet(lang_it->second));

  return 0;
}
