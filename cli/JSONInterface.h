// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_CLI_INTERFACE_H_
#define MUMBLE_JSONBRIDGE_CLI_INTERFACE_H_

#include <mumble/json_bridge/BridgeClient.h>
#include <mumble/json_bridge/NamedPipe.h>

#include <nlohmann/json.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace CLI {

		/**
		 * The interface used to communicate with the Mumble JSON bridge
		 */
		class JSONInterface {
		private:
			/**
			 * The timeout to use for read operations
			 */
			uint32_t m_readTimeout;
			/**
			 * The timeout to use for write operations
			 */
			uint32_t m_writeTimeout;
			/**
			 * The pipe that is being used by this interface to receive answers from the Bridge
			 */
			NamedPipe m_pipe;
			/**
			 * The ID the Bridge has assigned us
			 */
			client_id_t m_id;
			/**
			 * A secret key that we are using to proof our identity
			 */
			std::string m_secret;
			/**
			 * The Bridge's secret
			 */
			std::string m_bridgeSecret;

		public:
			explicit JSONInterface(uint32_t readTimeout = 1000, uint32_t m_writeTimeout = 100);
			~JSONInterface();

			/**
			 * Sends the given message to the Mumble JSON Bridge
			 *
			 * @param msg The message to be sent
			 * @returns The Bridge's response
			 */
			nlohmann::json process(nlohmann::json msg) const;
		};

	}; // namespace CLI
};     // namespace JsonBridge
};     // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_CLI_INTERFACE_H_

