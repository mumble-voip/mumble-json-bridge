# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

include(FindPython3Interpreter)

set(GENERATED_OPERATIONS_FILE "${CMAKE_CURRENT_BINARY_DIR}/handleOperation.cpp")

add_executable(mumble_json_bridge_cli
	main.cpp
	JSONInterface.cpp
	JSONInstruction.cpp
	"${GENERATED_OPERATIONS_FILE}"
)

target_link_libraries(mumble_json_bridge_cli PUBLIC json_bridge)

find_package(Boost COMPONENTS program_options REQUIRED)
target_link_libraries(mumble_json_bridge_cli PUBLIC ${Boost_LIBRARIES})
target_include_directories(mumble_json_bridge_cli PUBLIC ${Boost_INCLUDE_DIRS} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

findPython3Interpreter(PYTHON_EXE)

# Generate the code for handling operations in the CLI
add_custom_command(
	OUTPUT "${GENERATED_OPERATIONS_FILE}"
	COMMAND "${PYTHON_EXE}" "${CMAKE_SOURCE_DIR}/scripts/generate_CLI_operations.py" -i "${CMAKE_CURRENT_SOURCE_DIR}/operations" -o "${GENERATED_OPERATIONS_FILE}"
)
