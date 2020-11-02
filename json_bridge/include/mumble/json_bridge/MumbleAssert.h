// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_ASSERT_H_
#define MUMBLE_JSONBRIDGE_ASSERT_H_

/**
 * Function called whenever an assertion fails
 *
 * @see MUMBLE_ASSERT
 */
void mumble_jsonbridge_assertionFailure(const char *message, const char *function, const char *file, int line);

#define MUMBLE_ASSERT(condition) MUMBLE_ASSERT_MSG(condition, "Failed condition is \"" #condition "\"")

#define MUMBLE_ASSERT_MSG(condition, msg)                                          \
	if (!(condition)) {                                                            \
		mumble_jsonbridge_assertionFailure(msg, __FUNCTION__, __FILE__, __LINE__); \
	}

#endif // MUMBLE_JSONBRIDGE_ASSERT_H_
