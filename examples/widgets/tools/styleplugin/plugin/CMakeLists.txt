# Generated from plugin.pro.

cmake_minimum_required(VERSION 3.14)
project(simplestyleplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/styleplugin/styles")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_plugin(simplestyleplugin)
target_sources(simplestyleplugin PRIVATE
    simplestyle.cpp simplestyle.h
    simplestyleplugin.cpp simplestyleplugin.h
)
set_target_properties(simplestyleplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(simplestyleplugin PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS simplestyleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
