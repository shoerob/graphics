#include "catch.hpp"

#include "Graphics/Engine.h"

using namespace CR::Graphics;

TEST_CASE("engine creation/destruction", "") {
	EngineSettings settings;
	settings.ApplicationName    = "Unit Test";
	settings.ApplicationVersion = 1;
	settings.EnableDebug        = true;

	REQUIRE_NOTHROW(CreateEngine(settings));
	REQUIRE_NOTHROW(ShutdownEngine());
}
