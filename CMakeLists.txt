cmake_minimum_required(VERSION 3.15)
project(stringbench)

set(CMAKE_CXX_STANDARD 17)

# required for clang completion in editors - must be set after creating project
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -march=sandybridge")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -march=sandybridge")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=sandybridge")

add_subdirectory("3rdParty/fmt")

add_executable(stringbench
  src/stringbench.cpp
)

target_link_libraries(stringbench PRIVATE fmt::fmt-header-only)

