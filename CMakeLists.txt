cmake_minimum_required(VERSION 3.25)
project(bouncy-bird LANGUAGES CXX)
include(CTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 23)

configure_file(asset/font/Exo-Regular.ttf
	${CMAKE_BINARY_DIR}/bin/main_font.ttf
	COPYONLY
)

set(BIRD_TEXTURE_PATH "${CMAKE_BINARY_DIR}/bin/BirdSprite.png")
configure_file(asset/img/BirdSprite.png
	${BIRD_TEXTURE_PATH}
	COPYONLY
)

add_subdirectory(src)
