// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "gtest/gtest.h"

#include <mumble/json_bridge/NamedPipe.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>

#define PIPENAME "testPipe"

#ifdef PLATFORM_UNIX
#	define PIPEDIR "."
#else
#	define PIPEDIR "\\\\.\\pipe\\"
#endif

#define TEST_STRING                                                                                           \
	"This is a test-string that should exceed the default pipe-buffer and should therefore require multiple " \
	"iterations for reading"

#define TEST_STRING_L32 "This is a string with 32 chars. "

constexpr unsigned int READ_TIMOUT = 10 * 1000;

using namespace Mumble::JsonBridge;

const std::filesystem::path pipePath = std::filesystem::path(PIPEDIR) / PIPENAME;


class PipeIOTest : public ::testing::Test {
protected:
	NamedPipe m_testPipe;
	std::atomic< bool > m_failed = std::atomic< bool >(false);
	boost::thread m_readerThread;

	PipeIOTest() {
		std::error_code errorCode;
		EXPECT_FALSE(std::filesystem::exists(pipePath, errorCode)) << "Pipe already exists";
		EXPECT_FALSE(errorCode) << "Checking for pipe-existance failed";
	}

	~PipeIOTest() {
		// Explicitly destroy the pipe in order to be able to check its effects
		// Note that this will not cause any problems with the object's actual destruction
		m_testPipe.destroy();

		std::error_code errorCode;
		EXPECT_FALSE(std::filesystem::exists(pipePath, errorCode) && !errorCode)
			<< "NamedPipe's destructor didn't destroy the pipe";
	}

	void setupPipeReader() {
		try {
			m_testPipe = NamedPipe::create(pipePath);

			ASSERT_EQ(m_testPipe.getPath(), pipePath);

			test_read();
		} catch (const boost::thread_interrupted &) {
			std::cout << "Pipe-thread was interrupted" << std::endl;
		} catch (const std::exception &e) {
			m_failed.store(true);
			FAIL() << "Exception: " << e.what() << std::endl;
		} catch (...) {
			m_failed.store(true);
			FAIL() << "Unhandled exception" << std::endl;
		}
	}

	virtual void test_read() const { ASSERT_EQ(m_testPipe.read_blocking(READ_TIMOUT), TEST_STRING); }

	void SetUp() override { m_readerThread = boost::thread(&PipeIOTest::setupPipeReader, this); }

	void TearDown() override { m_readerThread.join(); }
};

class PipeIOTest2 : public PipeIOTest {
	void test_read() const override { ASSERT_EQ(m_testPipe.read_blocking(READ_TIMOUT), TEST_STRING_L32); }
};

class PipeIOTest3 : public PipeIOTest {
	void test_read() const override {
		std::string content;
		ASSERT_THROW(content = m_testPipe.read_blocking(100), TimeoutException);
	}
};


void waitUntilPipeExists(const std::atomic< bool > &errorFlag) {
	// Busy waiting with sleep until the new thread spun up and the pipe is ready
	while (!std::filesystem::exists(pipePath) && !errorFlag.load()) {
		boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
	}
}

TEST_F(PipeIOTest, basicIO) {
	waitUntilPipeExists(m_failed);

	if (m_failed.load()) {
		std::cerr << "Test aborted" << std::endl;
		return;
	}

	NamedPipe::write(pipePath, TEST_STRING);
}

TEST_F(PipeIOTest2, contentMatchesBufferSize) {
	waitUntilPipeExists(m_failed);

	ASSERT_TRUE(strlen(TEST_STRING_L32) == 32);

	if (m_failed.load()) {
		std::cerr << "Test aborted" << std::endl;
		return;
	}

	NamedPipe::write(pipePath, TEST_STRING_L32);
}

TEST_F(PipeIOTest, interruptable) {
	boost::this_thread::sleep_for(boost::chrono::seconds(3));

	m_readerThread.interrupt();

	m_readerThread.join();

	SUCCEED();
}

TEST_F(PipeIOTest3, read_timeout) {
	// We don't write anything to the pipe and therefore expect the pipe's read function to throw a TimeoutException
	m_readerThread.join();

	SUCCEED();
}

TEST(PipeIOTest4, write_timeout_nonExistantTarget) {
	std::filesystem::path dummyPipePath(std::filesystem::path(PIPEDIR) / "myDummyPipe");
	ASSERT_THROW(NamedPipe::write(dummyPipePath, "dummyMsg", 100), TimeoutException);
}

TEST(PipeIOTest4, write_timeout_pipeNotDrained) {
	NamedPipe pipe = NamedPipe::create(std::filesystem::path(PIPEDIR) / "undrainedPipe");

	ASSERT_THROW(pipe.write("dummyMsg", 100), TimeoutException);
}
