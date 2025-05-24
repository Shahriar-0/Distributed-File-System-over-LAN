cmake_minimum_required(VERSION 3.16)

project(client-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick Network REQUIRED)

set(COMMON_SOURCES
    ../common/EncodingUtils.cpp
)

set(COMMON_HEADERS
    ../common/EncodingUtils.h
)

qt_add_executable(appclient-gui
    main.cpp
    ${COMMON_SOURCES}
)

target_include_directories(appclient-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

qt_add_qml_module(appclient-gui
    URI client-gui
    VERSION 1.0
    QML_FILES main.qml
    SOURCES
        client.h
        client.cpp
        ${COMMON_HEADERS}
)

set_target_properties(appclient-gui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appclient-gui
    PRIVATE Qt6::Quick Qt6::Network
)

include(GNUInstallDirs)
install(TARGETS appclient-gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
