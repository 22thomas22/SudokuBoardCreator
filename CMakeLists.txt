cmake_minimum_required(VERSION 4.2)
project(sodoku)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

add_executable(sodoku
        main.cpp
        )
