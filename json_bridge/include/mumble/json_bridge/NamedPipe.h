// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_NAMEDPIPE_H_
#define MUMBLE_JSONBRIDGE_NAMEDPIPE_H_

#include "mumble/json_bridge/NonCopyable.h"

#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

#ifdef PLATFORM_WINDOWS
#	include <windows.h>
#endif

namespace Mumble {
namespace JsonBridge {
	/**
	 * An exception thrown by NamedPipe operations if something didn't go as intended
	 *
	 * @tparam error_code_t The type of the underlying error code
	 */
	template< typename error_code_t > class PipeException : public std::exception {
	private:
		/**
		 * The error code associated with the encountered error
		 */
		error_code_t m_errorCode;
		/**
		 * A message for displaying purpose that explains what happened
		 */
		std::string m_message;

	public:
		/**
		 * @param errorCode The encountered error code
		 * @param context The context in which this error code has been encountered. This will be embedded in this
		 * exception's message
		 */
		PipeException(error_code_t errorCode, const std::string &context) : m_errorCode(errorCode) {
			m_message =
				std::string("Pipe action \"") + context + "\" returned error code " + std::to_string(m_errorCode);
		}

		const char *what() const noexcept { return m_message.c_str(); }
	};

	/**
	 * An exception thrown when an operation takes longer than allowed
	 */
	class TimeoutException : public std::exception {
	public:
		const char *what() const noexcept { return "TimeoutException"; }
	};

	/**
	 * Wrapper class around working with NamedPipes. Its main purpose is to abstract away the implementation differences
	 * between different platforms (e.g. Windows vs Posix-compliant systems).
	 * At the same time it serves as a RAII wrapper.
	 */
	class NamedPipe : NonCopyable {
	private:
		/**
		 * The path to the wrapped pipe
		 */
		std::filesystem::path m_pipePath;

#ifdef PLATFORM_WINDOWS
		/**
		 * On Windows this holds the handle to the pipe. On other platforms this variable doesn't exist.
		 */
		HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif

		/**
		 * Instantiates this wrapper. On Windows the m_handle member variable has to be set
		 * explicitly after having constructed this object.
		 *
		 * @param The path to the pipe that should be wrapped by this object
		 */
		explicit NamedPipe(const std::filesystem::path &path);

	public:
		/**
		 * Creates an empty (invalid) instance
		 */
		NamedPipe() = default;
		~NamedPipe();

		NamedPipe(NamedPipe &&other);
		NamedPipe &operator=(NamedPipe &&other);

		/**
		 * Creates a new named pipe at the specified location. If such a pipe (or other file) already exists at the
		 * given location, this function will fail.
		 *
		 * @param pipePath The path at which the pipe shall be created
		 * @returns A NamedPipe object wrapping the newly created pipe
		 */
		[[nodiscard]] static NamedPipe create(const std::filesystem::path &pipePath);
		/**
		 * Writes a message to the named pipe at the given location
		 *
		 * @param pipePath The path at which the pipe is expected to exist. If the pipe does not exist, the function
		 * will poll for its existance until it times out.
		 * @param content The messages that should be written to the named pipe
		 * @param timeout How long this function is allowed to take in milliseconds. Note that the timeout is only
		 * respected very roughly (especially on Windows) and should therefore rather be used to specify the general
		 * order of magnitude of the timeout instead of the exact timeout-interval.
		 */
		static void write(const std::filesystem::path &pipePath, const std::string &content,
						  unsigned int timeout = 1000);

		/**
		 * @returns Whether a named pipe at the given path currently exists
		 */
		static bool exists(const std::filesystem::path &pipePath);

		/**
		 * Writes to the named pipe wrapped by this object by calling NamedPipe::write
		 * @content The message that should be written to the named pipe
		 * @content How long this function is allowed to take in milliseconds. The remarks from NamedPipe::write apply.
		 *
		 * @see Mumble::JsonBridge::NamedPipe::write()
		 */
		void write(const std::string &content, unsigned int timeout = 1000) const;

		/**
		 * Reads content from the wrapped named pipe. This function will block until there is content available or the
		 * timeout is over. Once started this function will read all available content until EOF in a single block.
		 *
		 * @param timeout How long this function may wait for content. Note that this will not be respected precisely.
		 * Rather this specifies the general order of magnitude of the timeout.
		 * @returns The read content
		 */
		[[nodiscard]] std::string
			read_blocking(unsigned int timeout = (std::numeric_limits< unsigned int >::max)()) const;

		/**
		 * @returns The path of the wrapped named pipe
		 */
		[[nodiscard]] std::filesystem::path getPath() const noexcept;

		/**
		 * Destroys the wrapped named pipe. After having called this function the pipe does no
		 * longer exist in the OS's filesystem and this wrapper will become unusable.
		 *
		 * @note This function is called automatically by the object's destructor
		 * @note Calling this function multiple times is allowed. All but the first invocation are turned into no-opts.
		 */
		void destroy();

		/**
		 * @returns Whether this wrapper is currently in a valid state
		 */
		operator bool() const noexcept;
	};

}; // namespace JsonBridge
}; // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_NAMEDPIPE_H_
