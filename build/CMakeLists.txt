cmake_minimum_required (VERSION 2.6)

project (Graphics)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /WX /W4 /std:c++latest /fp:fast /fp:except-")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX /Oi /Ot /sdl-")

###############################################
#options
###############################################
option(IncludeTests "Include Unit Tests" ON)

include_directories("${PROJECT_SOURCE_DIR}/../../3rdParty/catch")
include (${CMAKE_CURRENT_SOURCE_DIR}/build.cmake)
include (${CMAKE_CURRENT_SOURCE_DIR}/../../core/build/build.cmake)
include (${CMAKE_CURRENT_SOURCE_DIR}/../../math/build/build.cmake)
