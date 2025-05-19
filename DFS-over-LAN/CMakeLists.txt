cmake_minimum_required(VERSION 3.16)
project(DFS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Qml)

# 1) Common noise+FEC
add_subdirectory(common)

# 2) Master Server
add_subdirectory(master)

# 3) Chunk Server
add_subdirectory(chunk)

# 4) GUI Client
add_subdirectory(client-gui)
