// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.


#ifndef MUMBLE_JSONBRIDGE_NONCOPYABLE_H_
#define MUMBLE_JSONBRIDGE_NONCOPYABLE_H_

namespace Mumble {
namespace JsonBridge {

	/**
	 * Helper class that disables copy-constructors. Extending this class will prevent the auto-generation of copy-constructors
	 * of the subclass.
	 */
	class NonCopyable {
	public:
		NonCopyable() = default;

		NonCopyable(NonCopyable &&) = default;
		NonCopyable &operator=(NonCopyable &&) = default;

		// Delete copy-constructor
		NonCopyable(const NonCopyable &) = delete;
		// Delete copy-assignment
		NonCopyable &operator=(const NonCopyable &) = delete;
	};

}; // namespace JsonBridge
}; // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_NONCOPYABLE_H_
