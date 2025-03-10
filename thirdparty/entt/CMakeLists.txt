# SegsEngine: removed docs , testing code and conan

#
# EnTT
#

cmake_minimum_required(VERSION 3.12.4)

#
# Building in-tree is not allowed (we take care of your craziness).
#

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()

#
# Read project version
#

set(ENTT_VERSION_REGEX "#define ENTT_VERSION_.*[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/entt/config/version.h" ENTT_VERSION REGEX ${ENTT_VERSION_REGEX})
list(TRANSFORM ENTT_VERSION REPLACE ${ENTT_VERSION_REGEX} "\\1")
string(JOIN "." ENTT_VERSION ${ENTT_VERSION})

#
# Project configuration
#

project(
    EnTT
    VERSION ${ENTT_VERSION}
    DESCRIPTION "Gaming meets modern C++ - a fast and reliable entity-component system (ECS) and much more"
    HOMEPAGE_URL "https://github.com/skypjack/entt"
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(VERBOSE "*")
message(VERBOSE "* ${PROJECT_NAME} v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")
message(VERBOSE "* Copyright (c) 2017-2020 Michele Caini <michele.caini@gmail.com>")
message(VERBOSE "*")

option(USE_LIBCPP "Use libc++ by adding -stdlib=libc++ flag if availbale." OFF)
option(USE_ASAN "Use address sanitizer by adding -fsanitize=address -fno-omit-frame-pointer flags" OFF)
option(USE_COMPILE_OPTIONS "Use compile options from EnTT." ON)

#
# Compiler stuff
#
set(HAS_LIBCPP FALSE)
if(NOT WIN32 AND USE_LIBCPP)
    include(CheckCXXSourceCompiles)
    include(CMakePushCheckState)

    cmake_push_check_state()

    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -stdlib=libc++")

    check_cxx_source_compiles("
        #include<type_traits>
        int main() { return std::is_same_v<int, char>; }
    " HAS_LIBCPP)

    if(NOT HAS_LIBCPP)
        message(VERBOSE "The option USE_LIBCPP is set (by default) but libc++ is not available. The flag will not be added to the target.")
    endif()

    cmake_pop_check_state()
endif()

#
# Add EnTT target
#

include(GNUInstallDirs)

add_library(EnTT INTERFACE)
add_library(EnTT::EnTT ALIAS EnTT)

target_include_directories(
    EnTT
    INTERFACE
        $<BUILD_INTERFACE:${EnTT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


if(USE_ASAN)
    target_compile_options(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>)
    target_link_libraries(EnTT INTERFACE $<$<CONFIG:Debug>:-fsanitize=address -fno-omit-frame-pointer>)
endif()

if(HAS_LIBCPP)
    target_compile_options(EnTT BEFORE INTERFACE -stdlib=libc++)
endif()

target_compile_features(EnTT INTERFACE cxx_std_17)

#
# Install EnTT
#

include(CMakePackageConfigHelpers)

install(
    TARGETS EnTT
    EXPORT EnTTTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

write_basic_package_version_file(
    EnTTConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${EnTT_SOURCE_DIR}/cmake/in/EnTTConfig.cmake.in
    EnTTConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
)

export(
    EXPORT EnTTTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/EnTTTargets.cmake
    NAMESPACE EnTT::
)

install(
    EXPORT EnTTTargets
    FILE EnTTTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
    NAMESPACE EnTT::
)

install(
    FILES
        ${PROJECT_BINARY_DIR}/EnTTConfig.cmake
        ${PROJECT_BINARY_DIR}/EnTTConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/EnTT/cmake
)

install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(PACKAGE EnTT)
