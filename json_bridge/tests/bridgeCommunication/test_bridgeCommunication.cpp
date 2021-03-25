// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "gtest/gtest.h"

#include <mumble/json_bridge/Bridge.h>
#include <mumble/json_bridge/NamedPipe.h>

#include "API_v_1_0_x_mock.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <vector>

using namespace Mumble::JsonBridge;

#ifdef PLATFORM_UNIX
#	define PIPEDIR "."
#else
#	define PIPEDIR "\\\\.\\pipe\\"
#endif

#define ASSERT_FIELD(msg, name, type)                                                       \
	ASSERT_TRUE(msg.contains(name)) << "Field does not contain a \"" << name << "\" field"; \
	ASSERT_TRUE(msg[name].is_##type()) << "Field \"" << name << "\" is not of type " #type

#define ASSERT_API_CALL_HAPPENED(funcName, amount)                                                       \
	ASSERT_TRUE(API_Mock::calledFunctions.count(funcName) > 0)                                           \
		<< "Expected an API call to \"" funcName "\" to have happened, but it didn't.";                  \
	ASSERT_EQ(API_Mock::calledFunctions[funcName], amount)                                               \
		<< "Expected API call to \"" funcName "\" to have happened " #amount " time(s) but it happened " \
		<< API_Mock::calledFunctions[funcName] << " time(s)";                                            \
	API_Mock::calledFunctions.erase(funcName);

const std::filesystem::path clientPipePath(std::filesystem::path(PIPEDIR) / ".client-pipe");

constexpr unsigned int READ_TIMEOUT = 5 * 1000;

const std::string clientSecret = "superSecureClientSecret";

class BridgeCommunication : public ::testing::Test {
protected:
	MumbleAPI m_api;
	Bridge m_bridge;
	NamedPipe m_clientPipe;
	std::string m_bridgeSecret;

	BridgeCommunication() : m_api(API_Mock::getMumbleAPI_v_1_0_x(), API_Mock::pluginID), m_bridge(m_api) {}

	void SetUp() override {
		ASSERT_FALSE(NamedPipe::exists(clientPipePath)) << "There already exists an old pipe";

		m_bridgeSecret.clear();
		m_clientPipe = NamedPipe::create(clientPipePath);
		m_bridge.start();
	}

	void TearDown() override {
		// Drain potential left-over messages
		// If there are no left-overs, then the function will time out and throw an exception
		std::string content;
		ASSERT_THROW(content = m_clientPipe.read_blocking(5), TimeoutException)
			<< "There are unread messages in the client-pipe";
		ASSERT_TRUE(content.size() == 0);

		m_bridge.stop(true);

		if (API_Mock::calledFunctions.size() > 0) {
			for (const auto &current : API_Mock::calledFunctions) {
				std::cerr << "Got " << current.second << " unexpected API call(s) to \"" << current.first << "\""
						  << std::endl;
			}

			// Clear calls in order to not mess with the following test
			API_Mock::calledFunctions.clear();

			FAIL() << "There were unexpected API function calls";
		}

		m_clientPipe.destroy();

		// For some reason using ASSERT_FALSE(...) << "msg" throws an SEH on windows
		// The circumstances of this are very odd but seem to somehow relate to the
		// test case for the invalid JSON. There were a couple of SEH errors in gtest
		// in the past though and since I was unable to find any hints of an error in
		// the code here, let's just assume that this is some sort of weird bug in gtest.
		if (NamedPipe::exists(clientPipePath)) {
			std::cerr << "Client pipe was not destroyed!" << std::endl;
			FAIL();
		}
		if (NamedPipe::exists(Bridge::s_pipePath)) {
			std::cerr << "Bridge pipe was not destroyed!" << std::endl;
			FAIL();
		}
	}

	void performRegistration() {
		// clang-format off
		nlohmann::json message = {
			{"message_type", "registration"},
			{"message",
				{
					{"pipe_path", clientPipePath.string()},
					{"secret", clientSecret}
				}
			}
		};
		// clang-format off

		NamedPipe::write(m_bridge.s_pipePath, message.dump());
	}

	[[nodiscard]]
	int performRegistrationAndDrain() {
		performRegistration();

		// Drain the bridge's answer to the registration
		std::string strAnswer = m_clientPipe.read_blocking();

		nlohmann::json answer = nlohmann::json::parse(strAnswer);

		m_bridgeSecret = answer["secret"].get<std::string>();

		return answer["response"]["client_id"].get<int>();
	}

	void checkAnswer(const nlohmann::json &answer) {
		ASSERT_TRUE(answer.is_object()) << "Answer is not an object";
		ASSERT_FIELD(answer, "response_type", string);
		ASSERT_FIELD(answer, "secret", string);
		if (answer["response_type"].get<std::string>() != "disconnect") {
			// The disconnect message doesn't have a response body
			ASSERT_FIELD(answer, "response", object);
			ASSERT_EQ(answer.size(), 3) << "Answer contains wrong amount of fields";
		} else {
			ASSERT_EQ(answer.size(), 2) << "Answer contains wrong amount of fields";
		}

		if (m_bridgeSecret.size() > 0) {
			ASSERT_EQ(m_bridgeSecret, answer["secret"].get<std::string>()) << "Bridge used wrong secret";
		}
	}
};


TEST_F(BridgeCommunication, basic_registration) {
	performRegistration();

	std::string answer = m_clientPipe.read_blocking(READ_TIMEOUT);
	nlohmann::json answerJson = nlohmann::json::parse(answer);

	checkAnswer(answerJson);

	const nlohmann::json &responseObj = answerJson["response"];
	ASSERT_TRUE(responseObj.is_object()) << "Response is not an object";
	ASSERT_EQ(responseObj.size(), 1) << "Response contains wrong amount of fields";
	ASSERT_FIELD(responseObj, "client_id", number_integer);

	ASSERT_EQ(answerJson["response_type"], "registration");
}

TEST_F(BridgeCommunication, error_registrationWithNonExistentPipe) {
	// clang-format off
	nlohmann::json message = {
		{"message_type", "registration"},
		{"message",
			{
				{"pipe_path", (std::filesystem::path(PIPEDIR) / "NonExistentPipeName").string()},
				{"secret", clientSecret}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	// If the registration references the wrong pipe, we can't expect anything to be written to our pipe
	std::string answer;
	ASSERT_THROW(answer = m_clientPipe.read_blocking(100), TimeoutException);
}

TEST_F(BridgeCommunication, disconnect) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "disconnect"},
		{"client_id", clientID},
		{"secret", clientSecret},
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	nlohmann::json answer = nlohmann::json::parse(m_clientPipe.read_blocking(READ_TIMEOUT));

	checkAnswer(answer);

	// Now that we're unregistered again, the next write operation should simply time-out since
	// the Bridge doesn't know about our client anymore and therefore can't report any error to us
	// Just send the disconnect message a second time in order to serve as a dummy-message
	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string dummy;
	ASSERT_THROW(dummy = m_clientPipe.read_blocking(100), TimeoutException);
}

TEST_F(BridgeCommunication, getLocalUserID) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "getLocalUserID"},
				{"parameter", 
					{
						{"connection", API_Mock::activeConnetion}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "api_call");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "function", string);
	ASSERT_FIELD(response, "status", string);
	ASSERT_FIELD(response, "return_value", number_unsigned);

	ASSERT_EQ(response["function"].get< std::string >(), "getLocalUserID");
	ASSERT_EQ(response["status"].get< std::string >(), "executed");
	ASSERT_EQ(response["return_value"].get< unsigned int >(), API_Mock::localUserID);

	ASSERT_API_CALL_HAPPENED("getLocalUserID", 1);
}

TEST_F(BridgeCommunication, getAllUsers) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "getAllUsers"},
				{"parameter", 
					{
						{"connection", API_Mock::activeConnetion}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "api_call");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "function", string);
	ASSERT_FIELD(response, "status", string);
	ASSERT_FIELD(response, "return_value", array);

	ASSERT_EQ(response["function"].get< std::string >(), "getAllUsers");
	ASSERT_EQ(response["status"].get< std::string >(), "executed");

	std::vector< mumble_userid_t > users = response["return_value"].get< std::vector< mumble_userid_t > >();
	ASSERT_EQ(users.size(), 2);
	ASSERT_TRUE(std::find(users.begin(), users.end(), API_Mock::localUserID) != users.end());
	ASSERT_TRUE(std::find(users.begin(), users.end(), API_Mock::otherUserID) != users.end());

	ASSERT_API_CALL_HAPPENED("getAllUsers", 1);
	// freeMemory has to be called since the array allocated for the users needs explicit freeing (handled by the API
	// cpp wrapper automatically)
	ASSERT_API_CALL_HAPPENED("freeMemory", 1);
}

TEST_F(BridgeCommunication, getUserName) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "getUserName"},
				{"parameter", 
					{
						{"connection", API_Mock::activeConnetion},
						{"user_id", API_Mock::localUserID}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "api_call");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "function", string);
	ASSERT_FIELD(response, "status", string);
	ASSERT_FIELD(response, "return_value", string);

	ASSERT_EQ(response["function"].get< std::string >(), "getUserName");
	ASSERT_EQ(response["status"].get< std::string >(), "executed");
	ASSERT_EQ(response["return_value"].get< std::string >(), API_Mock::localUserName);

	ASSERT_API_CALL_HAPPENED("getUserName", 1);
	// freeMemory has to be called since the string allocated for the name needs explicit freeing (handled by the API
	// cpp wrapper automatically)
	ASSERT_API_CALL_HAPPENED("freeMemory", 1);
}

TEST_F(BridgeCommunication, findUserByName) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "findUserByName"},
				{"parameter", 
					{
						{"connection", API_Mock::activeConnetion},
						{"user_name", API_Mock::localUserName}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "api_call");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "function", string);
	ASSERT_FIELD(response, "status", string);
	ASSERT_FIELD(response, "return_value", number_unsigned);

	ASSERT_EQ(response["function"].get< std::string >(), "findUserByName");
	ASSERT_EQ(response["status"].get< std::string >(), "executed");
	ASSERT_EQ(response["return_value"].get< mumble_userid_t >(), API_Mock::localUserID);

	ASSERT_API_CALL_HAPPENED("findUserByName", 1);
}

TEST_F(BridgeCommunication, log) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "log"},
				{"parameter", 
					{
						{"message", "I am a dummy log-msg"}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "api_call");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "function", string);
	ASSERT_FIELD(response, "status", string);
	ASSERT_FALSE(response.contains("return_value"));

	ASSERT_EQ(response["function"].get< std::string >(), "log");
	ASSERT_EQ(response["status"].get< std::string >(), "executed");

	ASSERT_API_CALL_HAPPENED("log", 1);
}

TEST_F(BridgeCommunication, error_missingMessageType) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"dummy", 0}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);
	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the missing field
	ASSERT_TRUE(errorMsg.find("message_type") != std::string::npos);
}

TEST_F(BridgeCommunication, error_missingSecret) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"message",
			{
				{"dummy", 0}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);
	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the missing field
	ASSERT_TRUE(errorMsg.find("secret") != std::string::npos);
}

TEST_F(BridgeCommunication, error_WrongSecret) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", "I am wrong"},
		{"message",
			{
				{"dummy", 0}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);
	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the missing field
	ASSERT_TRUE(errorMsg.find("secret") != std::string::npos);
}

TEST_F(BridgeCommunication, error_WrongMessageType) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "I am wrong"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"dummy", 0}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);
	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the missing field
	ASSERT_TRUE(errorMsg.find("message_type") != std::string::npos);
}

TEST_F(BridgeCommunication, error_wrongParamCount) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "log"},
				{"parameter", 
					{
						{"message", "I am a dummy log-msg"},
						{"dummy", "I am too much"}
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);

	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the wrong param count
	ASSERT_TRUE(errorMsg.find("expects") != std::string::npos);
	ASSERT_TRUE(errorMsg.find("parameter") != std::string::npos);
}

TEST_F(BridgeCommunication, error_wrongParamType) {
	int clientID = performRegistrationAndDrain();

	// clang-format off
	nlohmann::json message = {
		{"message_type", "api_call"},
		{"client_id", clientID},
		{"secret", clientSecret},
		{"message",
			{
				{"function", "log"},
				{"parameter", 
					{
						{"message", 3},
					}
				}
			}
		}
	};
	// clang-format on

	NamedPipe::write(m_bridge.s_pipePath, message.dump());

	std::string strAnswer = m_clientPipe.read_blocking(READ_TIMEOUT);

	nlohmann::json answer = nlohmann::json::parse(strAnswer);

	checkAnswer(answer);

	ASSERT_EQ(answer["response_type"].get< std::string >(), "error");

	const nlohmann::json &response = answer["response"];

	ASSERT_FIELD(response, "error_message", string);

	std::string errorMsg = response["error_message"].get< std::string >();
	// Make sure the error message actually references the wrong param type
	ASSERT_TRUE(errorMsg.find("message") != std::string::npos);
	ASSERT_TRUE(errorMsg.find("expected") != std::string::npos);
	ASSERT_TRUE(errorMsg.find("string") != std::string::npos);
}

TEST_F(BridgeCommunication, error_invalidJSON) {
	int clientID = performRegistrationAndDrain();

	// Note the missing "}" at the end of the message
	NamedPipe::write(m_bridge.s_pipePath,
					 "{\"message\":{\"pipe_path\":\"./.client-pipe\"},\"message_type\":\"registration\"");

	std::string answer;
	ASSERT_THROW(answer = m_clientPipe.read_blocking(100), TimeoutException);
}
