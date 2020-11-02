# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

cmake_minimum_required(VERSION 3.10)

set(version "1.0.0")

project(mumble-json-bridge
	VERSION "${version}"
	DESCRIPTION "A Mumble plugin that offers a JSON API for Mumble interaction via named pipes"
	LANGUAGES "CXX"
)

if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
	message(STATUS "Building 64bit")
else()
	message(STATUS "Building non-64bit (probably 32bit)")
endif()


option(cli "Build the CLI" ON)
option(plugin "Build the Mumble plugin" ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_ImplicitConversions OFF CACHE INTERNAL "")

# Make sure that ctest can be run from the build dir directly and not only in certain sub-dirs inside of it
enable_testing()

add_subdirectory(3rdParty/nlohmann-json)
add_subdirectory(3rdParty/mumble-plugin-cpp)

add_subdirectory(json_bridge)

if (plugin)
	add_subdirectory(plugin)
endif()

if (cli)
	add_subdirectory(cli)
endif()
