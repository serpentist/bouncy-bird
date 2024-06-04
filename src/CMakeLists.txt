find_package(box2d REQUIRED)
find_package(SFML COMPONENTS system window network audio graphics REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_BUILD_TYPE "Release")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options(-O3)
else()
	add_compile_options(-O0 -g)
endif()

set(MAIN_FONT_PATH ${CMAKE_BINARY_DIR}/bin/main_font.ttf)
configure_file(main.cpp.in ${CMAKE_BINARY_DIR}/main.cpp @ONLY)
add_executable(run ${CMAKE_BINARY_DIR}/main.cpp)

target_link_libraries(run PRIVATE
	sfml-system
	sfml-audio
	sfml-network
	sfml-graphics
	sfml-window
	box2d
)

target_include_directories(run PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(run PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
