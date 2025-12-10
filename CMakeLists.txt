cmake_minimum_required(VERSION 3.15)

project(TrialServer)

add_executable(TrialServer server.cpp)

target_link_libraries(TrialServer PRIVATE pthread)