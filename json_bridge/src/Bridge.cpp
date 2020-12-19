// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/Bridge.h"
#include "mumble/json_bridge/NamedPipe.h"
#include "mumble/json_bridge/Util.h"

#include "mumble/json_bridge/messages/Message.h"
#include "mumble/json_bridge/messages/Registration.h"

#include <exception>
#include <filesystem>
#include <iostream>

#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>

#define CHECK_THREAD assert(m_workerThread.get_id() == boost::this_thread::get_id())


#ifdef PLATFORM_WINDOWS
#	define PIPE_DIR "\\\\.\\pipe\\"
#else
#	define PIPE_DIR "/tmp/"
#endif

namespace Mumble {
namespace JsonBridge {

	client_id_t Bridge::s_nextClientID = 0;
	const std::filesystem::path Bridge::s_pipePath(std::filesystem::path(PIPE_DIR) / ".mumble-json-bridge");

	Bridge::Bridge(const MumbleAPI &api) : m_api(api) {}

	void Bridge::doStart() {
		{
			// Make sure that m_workerThread has been assigned properly before accessing it
			std::lock_guard< std::mutex > guard(m_startMutex);
			CHECK_THREAD;
		}

		// Generate a secret that we are going to use
		m_secret = Util::generateRandomString(12);

		try {
			m_pipe = NamedPipe::create(s_pipePath);

			if (!m_pipe) {
				std::cerr << "Error creating pipe" << std::endl;
				return;
			}

			std::string content;
			// Loop until the thread is interrupted
			while (true) {
				content = m_pipe.read_blocking();
				std::cout << "Read from pipe:" << std::endl << content << std::endl;

				try {
					nlohmann::json message = nlohmann::json::parse(content);

					processMessage(message);
				} catch (const nlohmann::json::parse_error &e) {
					std::cerr << "Mumble-JSON-Bridge: Can't parse message: " << e.what() << std::endl;
				} catch (const TimeoutException &) {
					std::cerr << "Mumble-JSON-Bridge: NamedPipe IO timed out" << std::endl;
				}
			};

			std::cout << "Stopping pipe-query" << std::endl;
		} catch (const boost::thread_interrupted &) {
			// Destroy the client-pipe
			m_pipe.destroy();

			// rethrow
			throw;
		} catch (const std::exception &e) {
			// Destroy the client-pipe
			m_pipe.destroy();

			std::cerr << "Mumble-JSON-Bridge failed: " << e.what() << std::endl;

			// Exit thread
			return;
		}
	}

	void Bridge::processMessage(const nlohmann::json &msg) {
		CHECK_THREAD;

		std::cout << "Message: " << msg.dump() << std::endl;

		client_id_t id = INVALID_CLIENT_ID;

		try {
			Messages::MessageType type;
			try {
				type = Messages::parseBasicFormat(msg);
			} catch (const Messages::InvalidMessageException &) {
				// See if the message contains a client_id field as this would allow us to actually return
				// an error to the respective client instead of simply writing something to cerr (which the
				// client won't see).
				if (msg.contains("client_id")) {
					id = msg["client_id"].get< client_id_t >();
				}

				// Rethrow original exception
				throw;
			}

			if (type != Messages::MessageType::REGISTRATION) {
				MESSAGE_ASSERT_FIELD(msg, "client_id", number_integer);

				id = msg["client_id"].get< client_id_t >();

				MESSAGE_ASSERT_FIELD(msg, "secret", string);

				auto it = m_clients.find(id);

				if (it == m_clients.end()) {
					throw Messages::InvalidMessageException("Invalid client ID");
				}

				if (!it->second.secretMatches(msg["secret"].get< std::string >())) {
					throw Messages::InvalidMessageException("Permission denied (invalid secret)");
				}
			}

			switch (type) {
				case Messages::MessageType::REGISTRATION:
					handleRegistration(Messages::Registration(msg["message"]));
					break;
				case Messages::MessageType::API_CALL:
					handleAPICall(m_clients[id], Messages::APICall(m_api, msg["message"]));
					break;
				case Messages::MessageType::DISCONNECT:
					handleDisconnect(msg);
					break;
			}
		} catch (const Messages::InvalidMessageException &e) {
			if (id != INVALID_CLIENT_ID && m_clients.find(id) != m_clients.end()) {
				const BridgeClient &client = m_clients[id];

				// clang-format off
				nlohmann::json errorMsg = {
					{ "response_type", "error" },
					{ "secret", m_secret },
					{ "response", 
						{
							{ "error_message", std::string(e.what()) }
						}
					}
				};
				// clang-format on

				client.write(errorMsg.dump());
			} else {
				std::cerr << "Mumble-JSON-Bridge: Got error for unknown client: " << e.what() << std::endl;
			}
		}
	}

	void Bridge::handleRegistration(const Messages::Registration &msg) {
		CHECK_THREAD;

		std::error_code errorCode;
		if (std::filesystem::exists(msg.m_pipePath, errorCode)) {
			client_id_t id = s_nextClientID;
			s_nextClientID++;

			m_clients[id] = BridgeClient(msg.m_pipePath, msg.m_secret, id);

			// Tell the client about its assigned ID
			// clang-format off
			nlohmann::json response = {
				{ "response_type", "registration" },
				{ "secret", m_secret },
				{ "response",
					{
						{ "client_id", id }
					}
				}
			};
			// clang-format on

			m_clients[id].write(response.dump());
		}
	}

	void Bridge::handleAPICall(const BridgeClient &client, const Messages::APICall &msg) const {
		nlohmann::json response = msg.execute(m_secret);

		client.write(response.dump());
	}

	void Bridge::handleDisconnect(const nlohmann::json &msg) {
		client_id_t id = msg["client_id"].get< client_id_t >();
		// Move the client out of the list of known clients
		BridgeClient client = std::move(m_clients[id]);
		m_clients.erase(id);


		// clang-format off
		nlohmann::json response = {
			{ "response_type", "disconnect" },
			{ "secret", m_secret },
		};
		// clang-format on

		client.write(response.dump());
	}

	void Bridge::start() {
		// We need a mutex here in order to make sure that doStart won't start using m_workerThread before
		// it has been initialized properly by below statement.
		std::lock_guard< std::mutex > guard(m_startMutex);
		m_workerThread = boost::thread(&Bridge::doStart, this);
	}

	void Bridge::stop(bool join) {
		m_workerThread.interrupt();

		if (join) {
			m_workerThread.join();
		}
	}

}; // namespace JsonBridge
}; // namespace Mumble
