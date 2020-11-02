// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/Util.h"

#include <random>

namespace Mumble {
namespace JsonBridge {
	namespace Util {

		std::string generateRandomString(size_t size) {
			const char chars[] = "0123456789"
								 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
								 "abcdefghijklmnopqrstuvwxyz"
								 "+-*/()[]{}";

			std::string str;
			str.resize(size);

			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution< int > dist(0, sizeof(chars) - 1);

			for (size_t i = 0; i < size; i++) {
				str[i] = chars[dist(mt)];
			}

			return str;
		}

	}; // namespace Util
};     // namespace JsonBridge
};     // namespace Mumble
