cmake_minimum_required(VERSION 3.16)
project(noiseecc LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

# Pull in schifra as a header‐only library
include(FetchContent)
FetchContent_Declare(
  schifra
  GIT_REPOSITORY https://github.com/ArashPartow/schifra.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(schifra)

add_library(noiseecc INTERFACE)

target_include_directories(noiseecc INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${schifra_SOURCE_DIR}/include
)

# We depend on QtCore for QByteArray, QRandomGenerator, etc.
target_link_libraries(noiseecc INTERFACE Qt6::Core)
