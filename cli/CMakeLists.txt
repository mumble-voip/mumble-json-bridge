# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

add_executable(mumble_json_bridge_cli
	main.cpp
	JSONInterface.cpp
	JSONInstruction.cpp
	handleOperation.cpp
)

target_link_libraries(mumble_json_bridge_cli PUBLIC json_bridge)

find_package(Boost COMPONENTS program_options REQUIRED)
target_link_libraries(mumble_json_bridge_cli PUBLIC ${Boost_LIBRARIES})
target_include_directories(mumble_json_bridge_cli PUBLIC ${Boost_INCLUDE_DIRS})
