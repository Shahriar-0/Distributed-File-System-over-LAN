cmake_minimum_required(VERSION 3.16)

project(ChunkServer LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(COMMON_SOURCES
    ../common/EncodingUtils.cpp
)

set(COMMON_HEADERS
    ../common/EncodingUtils.h
)

add_executable(chunk_server
main.cpp
chunkServer.h
    chunkServer.cpp
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_include_directories(chunk_server PRIVATE ../common)

target_link_libraries(chunk_server Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

include(GNUInstallDirs)
install(TARGETS chunk_server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)