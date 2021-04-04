# Copyright 2021 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

# This function will attempt to find a Python3 interpreter. In constrast to the approach taken by
# find_package(Python3) this function will first try to get a suitable executable from the current
# PATH and does not try to find the most recent version of Python installed on this computer.
# This is so that ideally this function finds the same binary that is also used if the user typed
# e.g. "python3" in their command-line.
# If it fails to get the executable from the PATH, it falls back to using find_package after all.
function(findPython3Interpreter outVar)
	# Start by trying to search for "python3" executable in PATH
	find_program(PYTHON3_EXE NAMES "python3")

	if (NOT PYTHON3_EXE)
		# Try to search for "python" executable in PATH
		find_program(PYTHON3_EXE NAMES "python")

		if (PYTHON3_EXE)
			# Check that this is actually Python3 and not Python2
			execute_process(COMMAND "${PYTHON3_EXE}" "--version" OUTPUT_VARIABLE PYTHON3_VERSION)
			# Remove "Python" from version specifier
			string(REPLACE "Python" "" PYTHON3_VERSION "${PYTHON3_VERSION}")
			string(STRIP "${PYTHON3_VERSION}" PYTHON3_VERSION)

			if (NOT "${PYTHON3_VERSION}" MATCHES "^3")
				# this is not Python3 -> clear the vars
				set(PYTHON3_EXE "NOTFOUND")
			endif()

			set(PYTHON3_VERSION "NOTFOUND")
		endif()
	endif()

	if (NOT PYTHON3_EXE)
		message(WARNING "Can't find Python3 in PATH -> Falling back to find_package")
		find_package(Python3 COMPONENTS Interpreter QUIET)
		set(PYTHON3_EXE "${Python3_EXECUTABLE}")
	endif()

	if (NOT PYTHON3_EXE)
		message(FATAL_ERROR "Unable to find Python3 interpreter")
	else()
		message(STATUS "Found Python3: ${PYTHON3_EXE}")
		set(${outVar} "${PYTHON3_EXE}" PARENT_SCOPE)
	endif()
endfunction()
