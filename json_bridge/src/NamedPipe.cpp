// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/NamedPipe.h"
#include "mumble/json_bridge/MumbleAssert.h"
#include "mumble/json_bridge/NonCopyable.h"

#include <boost/thread/thread.hpp>

#ifdef PLATFORM_UNIX
#	include <fcntl.h>
#	include <unistd.h>
#	include <poll.h>
#	include <sys/stat.h>
#endif

#ifdef PLATFORM_WINDOWS
#	include <windows.h>
#endif

#include <fstream>
#include <iostream>

namespace Mumble {
namespace JsonBridge {
	template< typename handle_t, typename close_handle_function_t, handle_t invalid_handle, int successCode >
	class FileHandleWrapper : NonCopyable {
	private:
		handle_t m_handle;
		close_handle_function_t m_closeFunc;

	public:
		explicit FileHandleWrapper(handle_t handle, close_handle_function_t closeFunc)
			: m_handle(handle), m_closeFunc(closeFunc) {}

		explicit FileHandleWrapper() : m_handle(invalid_handle) {}

		~FileHandleWrapper() {
			if (m_handle != invalid_handle) {
				if (m_closeFunc(m_handle) != successCode) {
					std::cerr << "Failed at closing guarded handle" << std::endl;
				}
			}
		}

		// Be on the safe-side and delete move-constructor as it isn't explicitly implemented
		FileHandleWrapper(FileHandleWrapper &&) = delete;

		// Move-assignment operator
		FileHandleWrapper &operator=(FileHandleWrapper &&other) {
			// Move handle
			m_handle       = other.m_handle;
			other.m_handle = invalid_handle;

			// Copy over the close-func in case this instance was created using the default
			// constructor in which case the close-func is not specified.
			m_closeFunc = other.m_closeFunc;

			return *this;
		}

		handle_t &get() { return m_handle; }

		operator handle_t() { return m_handle; }

		bool operator==(handle_t other) { return m_handle == other; }
		bool operator!=(handle_t other) { return m_handle != other; }
		operator bool() { return m_handle != invalid_handle; }
	};

	constexpr int PIPE_WAIT_INTERVAL       = 10;
	constexpr int PIPE_WRITE_WAIT_INTERVAL = 5;
	constexpr int PIPE_BUFFER_SIZE         = 32;

#ifdef PLATFORM_UNIX
	using handle_t = FileHandleWrapper< int, int (*)(int), -1, 0 >;

	NamedPipe NamedPipe::create(const std::filesystem::path &pipePath) {
		std::error_code errorCode;
		MUMBLE_ASSERT(std::filesystem::is_directory(pipePath.parent_path(), errorCode) && !errorCode);

		// Create fifo that only the same user can read & write
		if (mkfifo(pipePath.c_str(), S_IRUSR | S_IWUSR) != 0) {
			throw PipeException< int >(errno, "Create");
		}

		return NamedPipe(pipePath);
	}

	void NamedPipe::write(const std::filesystem::path &pipePath, const std::string &content, unsigned int timeout) {
		handle_t handle;
		do {
			handle = handle_t(::open(pipePath.c_str(), O_WRONLY | O_NONBLOCK), &::close);

			if (!handle) {
				if (timeout > PIPE_WRITE_WAIT_INTERVAL) {
					timeout -= PIPE_WRITE_WAIT_INTERVAL;
					boost::this_thread::sleep_for(boost::chrono::milliseconds(PIPE_WRITE_WAIT_INTERVAL));
				} else {
					throw TimeoutException();
				}
			}
		} while (!handle);

		if (::write(handle, content.c_str(), content.size()) < 0) {
			throw PipeException< int >(errno, "Write");
		}
	}

	std::string NamedPipe::read_blocking(unsigned int timeout) const {
		std::string content;

		handle_t handle(::open(m_pipePath.c_str(), O_RDONLY | O_NONBLOCK), &::close);

		if (handle == -1) {
			throw PipeException< int >(errno, "Open");
		}

		pollfd pollData = { handle, POLLIN, 0 };
		while (::poll(&pollData, 1, PIPE_WAIT_INTERVAL) != -1 && !(pollData.revents & POLLIN)) {
			// Check if the thread has been interrupted
			boost::this_thread::interruption_point();

			if (timeout > PIPE_WAIT_INTERVAL) {
				timeout -= PIPE_WAIT_INTERVAL;
			} else {
				throw TimeoutException();
			}
		}

		char buffer[PIPE_BUFFER_SIZE];

		ssize_t readBytes;
		while ((readBytes = ::read(handle, &buffer, PIPE_BUFFER_SIZE)) > 0) {
			content.append(buffer, readBytes);
		}

		// 0 Means there is no more input, negative numbers indicate errors
		// If the error simply is EAGAIN this means that the message has been read completely
		// and a request for further data would block (since atm there is no more data available).
		if (readBytes == -1 && errno != EAGAIN) {
			throw PipeException< int >(errno, "Read");
		}

		return content;
	}
#endif

#ifdef PLATFORM_WINDOWS
	using handle_t = FileHandleWrapper< HANDLE, BOOL (*)(HANDLE), INVALID_HANDLE_VALUE, true >;

	void waitOnAsyncIO(HANDLE handle, LPOVERLAPPED overlappedPtr, unsigned int &timeout) {
		constexpr unsigned int pendingWaitInterval = 10;

		DWORD transferedBytes;
		BOOL result;
		while (!(result = GetOverlappedResult(handle, overlappedPtr, &transferedBytes, FALSE))
			   && GetLastError() == ERROR_IO_INCOMPLETE) {
			if (timeout > pendingWaitInterval) {
				timeout -= pendingWaitInterval;
			} else {
				throw TimeoutException();
			}

			boost::this_thread::sleep_for(boost::chrono::milliseconds(pendingWaitInterval));
		}

		if (!result) {
			throw PipeException< DWORD >(GetLastError(), "Waiting for pending IO");
		}
	}

	NamedPipe NamedPipe::create(const std::filesystem::path &pipePath) {
		MUMBLE_ASSERT(pipePath.parent_path() == "\\\\.\\pipe");

		HANDLE pipeHandle = CreateNamedPipe(pipePath.string().c_str(),
											PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
											PIPE_TYPE_BYTE | PIPE_WAIT,
											1,   // # of allowed pipe instances
											0,   // Size of outbound buffer
											0,   // Size of inbound buffer
											0,   // Use default wait time
											NULL // Use default security attributes
		);

		if (pipeHandle == INVALID_HANDLE_VALUE) {
			throw PipeException< DWORD >(GetLastError(), "Create");
		}

		NamedPipe pipe(pipePath);
		pipe.m_handle = pipeHandle;

		return pipe;
	}

	void NamedPipe::write(const std::filesystem::path &pipePath, const std::string &content, unsigned int timeout) {
		MUMBLE_ASSERT(pipePath.parent_path() == "\\\\.\\pipe");

		while (true) {
			// We can't use a timeout of 0 as this would be the special value NMPWAIT_USE_DEFAULT_WAIT causing
			// the function to use a default wait-time
			if (!WaitNamedPipe(pipePath.string().c_str(), 1)) {
				if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_SEM_TIMEOUT) {
					if (timeout > PIPE_WRITE_WAIT_INTERVAL) {
						timeout -= PIPE_WRITE_WAIT_INTERVAL;
					} else {
						throw TimeoutException();
					}

					// Decrease wait intverval by 1ms as this is the timeout we have waited on the pipe above already
					boost::this_thread::sleep_for(boost::chrono::milliseconds(PIPE_WRITE_WAIT_INTERVAL - 1));
				} else {
					throw PipeException< DWORD >(GetLastError(), "WaitNamedPipe");
				}
			} else {
				break;
			}
		}

		handle_t handle(
			CreateFile(pipePath.string().c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL),
			&CloseHandle);

		if (!handle) {
			throw PipeException< DWORD >(GetLastError(), "Open for write");
		}

		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(OVERLAPPED));
		if (!WriteFile(handle, content.c_str(), static_cast< DWORD >(content.size()), NULL, &overlapped)) {
			if (GetLastError() == ERROR_IO_PENDING) {
				waitOnAsyncIO(handle, &overlapped, timeout);
			} else {
				throw PipeException< DWORD >(GetLastError(), "Write");
			}
		}
	}

	void disconnectAndReconnect(HANDLE pipeHandle, LPOVERLAPPED overlappedPtr, bool disconnectFirst,
								unsigned int &timeout) {
		if (disconnectFirst) {
			if (!DisconnectNamedPipe(pipeHandle)) {
				throw PipeException< DWORD >(GetLastError(), "Disconnect");
			}
		}

		if (!ConnectNamedPipe(pipeHandle, overlappedPtr)) {
			switch (GetLastError()) {
				case ERROR_IO_PENDING:
					// Wait for async IO operation to complete
					waitOnAsyncIO(pipeHandle, overlappedPtr, timeout);

					return;

				case ERROR_NO_DATA:
				case ERROR_PIPE_CONNECTED:
					// These error codes mean that there is a client connected already.
					// In theory ERROR_NO_DATA means that the client has closed its handle
					// to the pipe already but it seems that we can read from the pipe just fine
					// so we count this as a success.
					return;
				default:
					throw PipeException< DWORD >(GetLastError(), "Connect");
			}
		}
	}

	std::string NamedPipe::read_blocking(unsigned int timeout) const {
		std::string content;

		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(OVERLAPPED));

		handle_t eventHandle(CreateEvent(NULL, TRUE, TRUE, NULL), &CloseHandle);
		overlapped.hEvent = eventHandle;

		// Connect to pipe
		disconnectAndReconnect(m_handle, &overlapped, false, timeout);

		// Reset overlapped structure
		memset(&overlapped, 0, sizeof(OVERLAPPED));
		overlapped.hEvent = eventHandle;

		char buffer[PIPE_BUFFER_SIZE];

		// Loop until we explicitly break from it (because we're done reading)
		while (true) {
			DWORD readBytes = 0;
			BOOL success    = ReadFile(m_handle, &buffer, PIPE_BUFFER_SIZE, &readBytes, &overlapped);
			if (!success && GetLastError() == ERROR_IO_PENDING) {
				// Wait for the async IO to complete (note that the thread can't be
				// interrupted while waiting this way)
				success = GetOverlappedResult(m_handle, &overlapped, &readBytes, TRUE);

				if (!success && GetLastError() != ERROR_BROKEN_PIPE) {
					throw PipeException< DWORD >(GetLastError(), "Overlapped waiting");
				}
			}

			if (!success && content.size() > 0) {
				// We have already read some data -> assume that we reached the end of it
				break;
			}

			if (success) {
				content.append(buffer, readBytes);

				if (readBytes < PIPE_BUFFER_SIZE) {
					// It seems like we read the complete message
					break;
				}
			} else {
				switch (GetLastError()) {
					case ERROR_BROKEN_PIPE:
						// Reset overlapped structure
						memset(&overlapped, 0, sizeof(OVERLAPPED));
						overlapped.hEvent = eventHandle;

						disconnectAndReconnect(m_handle, &overlapped, true, timeout);

						// Reset overlapped structure
						memset(&overlapped, 0, sizeof(OVERLAPPED));
						overlapped.hEvent = eventHandle;
						break;
					case ERROR_PIPE_LISTENING:
						break;
					default:
						throw PipeException< DWORD >(GetLastError(), "Read");
				}

				if (timeout > PIPE_WAIT_INTERVAL) {
					timeout -= PIPE_WAIT_INTERVAL;
				} else {
					throw TimeoutException();
				}

				boost::this_thread::sleep_for(boost::chrono::milliseconds(PIPE_WAIT_INTERVAL));
			}
		}

		DisconnectNamedPipe(m_handle);

		return content;
	}
#endif

	// Cross-platform implementations
	NamedPipe::NamedPipe(const std::filesystem::path &path) : m_pipePath(path) {}

#ifdef PLATFORM_WINDOWS
	NamedPipe::NamedPipe(NamedPipe &&other) : m_pipePath(std::move(other.m_pipePath)), m_handle(other.m_handle) {
		other.m_pipePath.clear();
		other.m_handle = INVALID_HANDLE_VALUE;
	}

	NamedPipe &NamedPipe::operator=(NamedPipe &&other) {
		m_pipePath = std::move(other.m_pipePath);
		m_handle   = other.m_handle;

		other.m_pipePath.clear();
		other.m_handle = INVALID_HANDLE_VALUE;

		return *this;
	}
#else  // PLATFORM_WINDOWS
	NamedPipe::NamedPipe(NamedPipe &&other) : m_pipePath(std::move(other.m_pipePath)) { other.m_pipePath.clear(); }

	NamedPipe &NamedPipe::operator=(NamedPipe &&other) {
		m_pipePath = std::move(other.m_pipePath);

		other.m_pipePath.clear();

		return *this;
	}
#endif // PLATFORM_WINDOWS

	NamedPipe::~NamedPipe() { destroy(); }

	std::filesystem::path NamedPipe::getPath() const noexcept { return m_pipePath; }

	void NamedPipe::destroy() {
		std::error_code errorCode;
		if (!m_pipePath.empty() && std::filesystem::exists(m_pipePath, errorCode)) {
#ifdef PLATFORM_WINDOWS
			if (!CloseHandle(m_handle)) {
				std::cerr << "Failed at closing pipe handle: " << GetLastError() << std::endl;
			}
#else
			std::filesystem::remove(m_pipePath, errorCode);

			if (errorCode) {
				std::cerr << "Failed at deleting pipe-object: " << errorCode << std::endl;
			}
#endif
			m_pipePath.clear();
		}
	}

	void NamedPipe::write(const std::string &content, unsigned int timeout) const {
		write(m_pipePath, content, timeout);
	}

	NamedPipe::operator bool() const noexcept { return !m_pipePath.empty(); }

}; // namespace JsonBridge
}; // namespace Mumble
