cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME NESEmulator)
project(${PROJECT_NAME})

set(${PROJECT_NAME}_HEADERS
	"Cartridge.h"
	"CPU.h"
	"FileHandle.h"
	"Mapper.h"
	"Mapper000.h"
	"Mapper001.h"
	"APU.h"
	"PPU.h"
	"RAM.h"
)

set(${PROJECT_NAME}_SOURCES
	"Cartridge.cpp"
	"CPU.cpp"
	"FileHandle.cpp"
	"Mapper.cpp"
	"Mapper001.cpp"
	"NESEmulator.cpp"
	"APU.cpp"
	"PPU.cpp"
	"RAM.cpp"
)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE PATH "Path to directory containing includes and libraries.")
find_path(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2 REQUIRED)
if(${CMAKE_EXE_LINKER_FLAGS} MATCHES "/machine:x64")
	find_library(SDL2_LIBRARY NAME SDL2 PATH_SUFFIXES x64 REQUIRED)
	find_library(SDL2_MAIN_LIBRARY NAME SDL2main PATH_SUFFIXES x64 REQUIRED)
else()
	find_library(SDL2_LIBRARY NAME SDL2 PATH_SUFFIXES x86 REQUIRED)
	find_library(SDL2_MAIN_LIBRARY NAME SDL2main PATH_SUFFIXES x86 REQUIRED)
endif()
get_filename_component(SDL2_LIBRARY_DIR ${SDL2_LIBRARY} DIRECTORY)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES} ${SDL2_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} ${SDL2_MAIN_LIBRARY})

# Copy Requisite DLLs to build directories.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_LIBRARY_DIR}/SDL2.dll" "${CMAKE_BINARY_DIR}/Debug")