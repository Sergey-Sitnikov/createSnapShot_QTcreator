cmake_minimum_required(VERSION 3.5)

project(Screen VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Укажите путь к CURL
set(CMAKE_PREFIX_PATH "/home/ssv/curl-7.84.0")
set(CMAKE_BUILD_TYPE Debug)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui)
find_package(CURL REQUIRED)

message(STATUS "CURL_LIBRARIES: ${CURL_LIBRARIES}")

set(PROJECT_SOURCES
    main.cpp
    snapshotapp.h
    snapshotapp.cpp
    capturethread.h
    capturethread.cpp
    MapObject.h
    MapObject.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Screen MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    if(ANDROID)
        add_library(Screen SHARED ${PROJECT_SOURCES})
    else()
        add_executable(Screen ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(Screen PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    curl # Явно указываем компоновщику искать libcurl
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Screen)
endif()

set_target_properties(Screen PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS Screen
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Screen)
endif()
