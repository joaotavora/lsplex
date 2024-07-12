#include <fmt/format.h>
#include <greeter/greeter.h>

namespace greeter {

Greeter::Greeter(std::string name) : _name(std::move(name)) {}

std::string Greeter::greet(LanguageCode lang) const {
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

}  // namespace greeter
