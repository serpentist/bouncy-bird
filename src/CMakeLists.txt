find_package(box2d REQUIRED)
find_package(SFML COMPONENTS system window network audio graphics REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	message(STATUS "Adding optimization at level 3")
	add_compile_options(-O3)
else()
	message(STATUS "Adding debug symbols")
	add_compile_options(-O0 -g)
endif()

add_executable(run main.cpp)
target_include_directories(run PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(run PRIVATE
	sfml-system
	sfml-audio
	sfml-network
	sfml-graphics
	sfml-window
	box2d
)

set(STAGING_DIR ${CMAKE_BINARY_DIR}/bouncybird)
set_target_properties(run PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${STAGING_DIR}
)

file(MAKE_DIRECTORY ${STAGING_DIR}/font)

file(COPY ${CMAKE_SOURCE_DIR}/asset/font/Exo-Regular.ttf
		DESTINATION ${STAGING_DIR}/font)

file(MAKE_DIRECTORY ${STAGING_DIR}/img)

file(COPY ${CMAKE_SOURCE_DIR}/asset/img/BirdSprite.png
		DESTINATION ${STAGING_DIR}/img)

install(DIRECTORY ${STAGING_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})
