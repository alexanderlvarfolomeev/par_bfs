cmake_minimum_required(VERSION 3.17)
project(par_bfs)

set(CMAKE_CXX_STANDARD 17)

link_directories(./lib)
include_directories(./include)
include_directories(./)

add_compile_options(-fopencilk)
add_link_options(-fopencilk)

add_executable(bench main.cpp bfs.cpp)
target_link_libraries(bench pthread)