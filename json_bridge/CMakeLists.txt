# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(tests "Build tests" OFF)

add_library(json_bridge
	STATIC
		src/NamedPipe.cpp
		src/Bridge.cpp
		src/MumbleAssert.cpp
		src/BridgeClient.cpp
		src/Util.cpp
		src/messages/Message.cpp
		src/messages/Registration.cpp
		src/messages/APICall.cpp
)

target_include_directories(json_bridge PUBLIC include/)

find_package(Boost COMPONENTS thread chrono REQUIRED)
target_link_libraries(json_bridge PUBLIC ${Boost_LIBRARIES})
target_include_directories(json_bridge PUBLIC ${Boost_INCLUDE_DIRS})

set(PLATFORM_DEFINES "")

if(UNIX)
	list(APPEND PLATFORM_DEFINES "PLATFORM_UNIX")
elseif(WIN32)
	list(APPEND PLATFORM_DEFINES "PLATFORM_WINDOWS")
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

target_link_libraries(json_bridge PUBLIC mumble_plugin_cpp_wrapper_api nlohmann_json::nlohmann_json)
target_compile_definitions(json_bridge PUBLIC ${PLATFORM_DEFINES})


if (tests)
	include(GoogleTest)
	add_subdirectory(tests)
endif()
