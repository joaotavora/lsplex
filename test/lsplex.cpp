#include <doctest/doctest.h>
#include <lsplex/lsplex.h>
#include <lsplex/version.h>

#include <string>

TEST_CASE("Lsplex version") {
  static_assert(std::string_view(LSPLEX_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(LSPLEX_VERSION) == std::string("1.0.0"));
}
