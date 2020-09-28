# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

# Prevent GTest from trying to install itself
set(INSTALL_GTEST FALSE CACHE BOOL "" FORCE)
# Force GTest to use shared CRT libraries which is what cmake defaults to for
# our test-cases anyways
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/3rdParty/googletest" "${CMAKE_BINARY_DIR}/3rdParty/googletest")
 
macro(create_test TESTNAME)
	# create an exectuable in which the tests will be stored
	add_executable(${TESTNAME} ${ARGN})

	# link the Google test infrastructure, mocking library, and a default main fuction to
	# the test executable.  Remove g_test_main if writing your own main function.
	target_link_libraries(${TESTNAME} PRIVATE gtest gmock gtest_main)

	# gtest_discover_tests replaces gtest_add_tests,
	# see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
	gtest_discover_tests(${TESTNAME}
		# set a working directory so your project root so that you can find test data via paths relative to the project root
		WORKING_DIRECTORY ${PROJECT_DIR}
		PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
	)

	# Signal the the files are built for testing purposes
	target_compile_definitions(${TESTNAME} PUBLIC TESTING)

	target_link_libraries(${TESTNAME} PRIVATE json_bridge)
endmacro()

add_subdirectory(pipeIO)
add_subdirectory(bridgeCommunication)
