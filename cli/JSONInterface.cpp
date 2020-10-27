// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "JSONInterface.h"

#include <mumble/json_bridge/Bridge.h>
#include <mumble/json_bridge/Util.h>

#include <filesystem>
#include <iostream>

namespace Mumble {
namespace JsonBridge {
	namespace CLI {

		JSONInterface::JSONInterface(uint32_t readTimeout, uint32_t writeTimeout)
			: m_readTimeout(readTimeout), m_writeTimeout(writeTimeout) {
			std::filesystem::path pipePath;
#ifdef PLATFORM_WINDOWS
			pipePath = "\\\\.\\pipe\\";
#else
			pipePath = "/tmp/";
#endif // PLATFORM_WINDOWS

			pipePath = pipePath / ".mumble-json-bridge-cli";

			m_pipe = NamedPipe::create(pipePath);

			m_secret = Util::generateRandomString(12);

			// clang-format off
			nlohmann::json registration = {
				{"message_type", "registration"},
				{"message",
					{
						{"pipe_path", pipePath.string()},
						{"secret", m_secret}
					}
				}
			};
			// clang-format off
			
			NamedPipe::write(Bridge::s_pipePath, registration.dump(), m_writeTimeout);

			nlohmann::json response = nlohmann::json::parse(m_pipe.read_blocking(m_readTimeout));

			m_bridgeSecret = response["secret"].get<std::string>();
			m_id = response["response"]["client_id"].get<client_id_t>();
		}

		JSONInterface::~JSONInterface() {
			// clang-format off
			nlohmann::json message = {
				{ "message_type", "disconnect" },
				{ "client_id", m_id },
				{ "secret", m_secret }
			};
			// clang-format on

			try {
				NamedPipe::write(Bridge::s_pipePath, message.dump(), m_writeTimeout);
				// We patiently wait for the Bridge's reply, even though we don't care about it. This is in
				// order for the Bridge's operation to not error due to timeout.
				std::string answer = m_pipe.read_blocking(m_readTimeout);
			} catch (...) {
				// Ignore any exceptions that this might cause. If it does throw then this client might not
				// be disconnected from the Bridge, which isn't that bad. Besides: We probably can't do anything
				// about it anyways, but we certainly don't want our program to terminate because of it.
			}
		}

		nlohmann::json JSONInterface::process(nlohmann::json msg) const {
			msg["secret"]    = m_secret;
			msg["client_id"] = m_id;

			NamedPipe::write(Bridge::s_pipePath, msg.dump(), m_writeTimeout);

			nlohmann::json response = nlohmann::json::parse(m_pipe.read_blocking(m_readTimeout));

			if (response["secret"].get< std::string >() != m_bridgeSecret) {
				std::cerr << "[ERROR]: Bridge secret doesn't match" << std::endl;
				return {};
			}

			// Remove the secret field as it has already been validated here
			response.erase("secret");

			return response;
		}
	}; // namespace CLI
};     // namespace JsonBridge
};     // namespace Mumble
