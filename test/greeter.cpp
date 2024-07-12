#include <doctest/doctest.h>
#include <greeter/greeter.h>
#include <greeter/version.h>

#include <string>

TEST_CASE("Greeter") {
  using namespace greeter;  // NOLINT

  Greeter greeter("Tests");

  CHECK(greeter.greet(LanguageCode::en) == "Hello, Tests!");
  CHECK(greeter.greet(LanguageCode::de) == "Hallo Tests!");
  CHECK(greeter.greet(LanguageCode::es) == "¡Hola Tests!");
  CHECK(greeter.greet(LanguageCode::fr) == "Bonjour Tests!");
}

TEST_CASE("Greeter version") {
  static_assert(std::string_view(GREETER_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(GREETER_VERSION) == std::string("1.0.0"));
}
