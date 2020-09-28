// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_UTILS_H_
#define MUMBLE_JSONBRIDGE_UTILS_H_

#include <string>

namespace Mumble {
namespace JsonBridge {
	namespace Util {

		/**
		 * Generates a random string consisting of alphanumeric
		 * characters and a few simple special characters.
		 *
		 * @param size The size of the string to generate
		 * @return The generated string
		 */
		std::string generateRandomString(size_t size);

	}; // namespace Util
}; // namespace JsonBridge
}; // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_UTILS_H_

