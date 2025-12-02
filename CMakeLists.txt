cmake_minimum_required(VERSION 3.15)

project(server)

add_executable(server server.cpp)

target_link_libraries(server PRIVATE pthread)