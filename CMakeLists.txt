cmake_minimum_required(VERSION 3.26)

project(minesweeper)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  message(FATAL_ERROR "The MSVC compiler is not supported.")
endif()

include(FetchContent)
find_package(Python3 COMPONENTS Interpreter)

if(NOT Python3_FOUND)
  message(FATAL_ERROR "Python was not found.")
endif()

set(CMAKE_C_STANDARD 99)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(raylib
  GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
  GIT_TAG "5.0"
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)

execute_process(
  COMMAND ${Python3_EXECUTABLE} ../generate_headers.py
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

file(GLOB_RECURSE SOURCES
  "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(minesweeper ${SOURCES})
target_link_libraries(minesweeper PRIVATE raylib)

add_custom_command(TARGET minesweeper POST_BUILD 
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
)