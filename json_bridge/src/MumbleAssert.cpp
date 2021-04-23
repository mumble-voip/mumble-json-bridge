// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/MumbleAssert.h"

#include <exception>
#include <iostream>


void mumble_jsonbridge_assertionFailure(const char *message, const char *function, const char *file, int line) {
	std::cerr << "Assertion failure in function " << function << " (" << file << ":" << line << "): " << message
			  << std::endl;
	std::terminate();
}
