cmake_minimum_required(VERSION 3.22)

project(simple_db)

set(CMAKE_CXX_STANDARD 17)

add_executable(
    server
    server.cpp
    utility.cpp
    utility.h
)

add_executable(
    client
    client.cpp
    utility.cpp
    utility.h
)