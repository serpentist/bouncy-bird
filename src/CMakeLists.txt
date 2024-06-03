add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_BUILD_TYPE "Release")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options(-O3)
else()
	add_compile_options(-O0 -g)
endif()

add_executable(run main.cpp)

target_link_libraries(run PRIVATE
	sfml-system
	sfml-audio
	sfml-network
	sfml-graphics
	sfml-window
)
