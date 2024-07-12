#include <fmt/core.h>
#include <greeter/greeter.h>
#include <greeter/version.h>

#include <cxxopts.hpp>
#include <string>
#include <unordered_map>

auto main(int argc, char **argv) -> int {
  const std::unordered_map<std::string, greeter::LanguageCode> languages{
      {"en", greeter::LanguageCode::en},
      {"de", greeter::LanguageCode::de},
      {"es", greeter::LanguageCode::es},
      {"fr", greeter::LanguageCode::fr},
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
    fmt::println("Greeter, version {}", GREETER_VERSION);
    return 0;
  }

  auto lang_it = languages.find(language);
  if (lang_it == languages.end()) {
    fmt::println("Unknown language code: {}", language);
    return 1;
  }

  greeter::Greeter greeter(name);
  fmt::println("{}", greeter.greet(lang_it->second));

  return 0;
}
