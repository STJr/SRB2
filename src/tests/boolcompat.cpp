#include <catch2/catch_test_macros.hpp>

#include "../doomtype.h"

TEST_CASE("C++ bool is convertible to doomtype.h boolean") {
	REQUIRE(static_cast<boolean>(true) == 1);
	REQUIRE(static_cast<boolean>(false) == 0);
}
