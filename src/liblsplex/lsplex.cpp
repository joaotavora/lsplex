#include <fmt/core.h>
#include <lsplex/lsplex.h>
#include <string>

namespace lsplex {

LsPlex::LsPlex(std::string name) : _name(std::move(name)) {}

std::string LsPlex::greet(LanguageCode lang) const {
  switch (lang) {
    default:
    case LanguageCode::en:
      return fmt::format("Hello, {}!", _name);
    case LanguageCode::de:
      return fmt::format("Hallo {}!", _name);
    case LanguageCode::es:
      return fmt::format("¡Hola {}!", _name);
    case LanguageCode::fr:
      return fmt::format("Bonjour {}!", _name);
  }
}

}  // namespace lsplex
