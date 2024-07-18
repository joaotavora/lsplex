#include <doctest/doctest.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>

#include <string>

TEST_CASE("LsPlex") {
  using namespace lsplex;  // NOLINT

  LsPlex lsplex("Tests");

  CHECK(lsplex.greet(LanguageCode::en) == "Hello, Tests!");
  CHECK(lsplex.greet(LanguageCode::de) == "Hallo Tests!");
  CHECK(lsplex.greet(LanguageCode::es) == "¡Hola Tests!");
  CHECK(lsplex.greet(LanguageCode::fr) == "Bonjour Tests!");
}

TEST_CASE("LsPlex version") {
  static_assert(std::string_view(LSPLEX_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(LSPLEX_VERSION) == std::string("1.0.0"));
}
