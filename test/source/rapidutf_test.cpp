#include <string>

#include "rapidutf/rapidutf.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Name is rapidutf", "[library]")
{
  auto const exported = exported_class {};
  REQUIRE(std::string("rapidutf") == exported.name());
}
