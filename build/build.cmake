find_package(Vulkan REQUIRED)

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}/../inc"
	$ENV{VULKAN_SDK}/include
)
link_directories(
	$ENV{VULKAN_SDK}/Lib
)

###############################################
#library
###############################################
set(GRAPHICS_PUBLIC_HDRS
    ${CMAKE_CURRENT_LIST_DIR}/../inc/Graphics/Engine.h
)

set(GRAPHICS_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../src/vulkan/Engine.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/vulkan/ErrorTranslation.h
    ${CMAKE_CURRENT_LIST_DIR}/../src/vulkan/ErrorTranslation.cpp
)

set(GRAPHICS_BUILD
    ${CMAKE_CURRENT_LIST_DIR}/../build/build.cmake
)

add_library(graphics OBJECT  ${GRAPHICS_PUBLIC_HDRS} ${GRAPHICS_SRCS} ${GRAPHICS_BUILD})
						
source_group("Public Headers" FILES ${GRAPHICS_PUBLIC_HDRS})
source_group("Source" FILES ${GRAPHICS_SRCS})
source_group("Build" FILES ${GRAPHICS_BUILD})
	
target_include_directories(graphics PRIVATE Vulkan::Vulkan)
				
if(IncludeTests)  
	###############################################
	#unit tests
	###############################################
set(GRAPHICS_TEST_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../tests/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../tests/Engine.cpp
)
	
	add_executable(graphics_tests  $<TARGET_OBJECTS:graphics>
						$<TARGET_OBJECTS:core>
						$<TARGET_OBJECTS:math>
						${GRAPHICS_TEST_SRCS})
	
	target_link_libraries(
		graphics_tests Vulkan::Vulkan
	)

	source_group("Source" FILES ${GRAPHICS_TEST_SRCS})
						
	set_property(TARGET graphics_tests APPEND PROPERTY FOLDER tests)

endif()
