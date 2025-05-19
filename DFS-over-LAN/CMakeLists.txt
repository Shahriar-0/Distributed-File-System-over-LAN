cmake_minimum_required(VERSION 3.16)
project(DFS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Qml)

add_subdirectory(common)

add_subdirectory(master)

add_subdirectory(chunk)

add_subdirectory(client-gui)
