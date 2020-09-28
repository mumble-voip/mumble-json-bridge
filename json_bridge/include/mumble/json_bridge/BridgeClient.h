// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_BRIDGECLIENT_H_
#define MUMBLE_JSONBRIDGE_BRIDGECLIENT_H_

#include "mumble/json_bridge/NonCopyable.h"

#include <filesystem>
#include <limits>
#include <string>

namespace Mumble {
namespace JsonBridge {

	/**
	 * The type used for representing client IDs
	 *
	 * @see Mumble::JsonBridge::INVALID_CLIENT_ID
	 */
	using client_id_t                       = unsigned int;
	/**
	 * The value representing an invalid client ID. This can be used to represent uninitialized IDs.
	 */
	constexpr client_id_t INVALID_CLIENT_ID = (std::numeric_limits< client_id_t >::max)();

	/**
	 * This class represents a client that is currently connected to the Bridge. In particular it wraps functionality
	 * like verifying a client's secret and writing messages to it.
	 *
	 * @see Mumble::JsonBridge::Bridge
	 */
	class BridgeClient : NonCopyable {
	private:
		/**
		 * The client's ID
		 */
		client_id_t m_id = INVALID_CLIENT_ID;
		/**
		 * The path to the client's named pipe. This is where messages are being written to.
		 */
		std::filesystem::path m_pipePath;
		/**
		 * The client's secret that it provided during registration. If a message is received claiming to
		 * to come from this client it has to be verified that the provided secret matches this one. Otherwise
		 * the identity of the client can't be verified.
		 *
		 * @see Mumble::JsonBridge::BridgeClient::secretMatches()
		 */
		std::string m_secret;

	public:
		/**
		 * Creates an **invalid** instance
		 */
		explicit BridgeClient() = default;
		/**
		 * Creates an instance of a client with the specified data.
		 *
		 * @param path The path to the client's named pipe
		 * @param secret The secret the client has provided (used for identity verification)
		 * @param id The ID that is assigned to this client. If not given, the ID of this client is set to be invalid.
		 */
		explicit BridgeClient(const std::filesystem::path &pipePath, const std::string &secret,
							  client_id_t id = INVALID_CLIENT_ID);
		~BridgeClient();

		BridgeClient(BridgeClient &&) = default;
		BridgeClient &operator=(BridgeClient &&) = default;

		/**
		 * Writes the given message to this client's named pipe
		 *
		 * @param message The message to write
		 */
		void write(const std::string &message) const;

		/**
		 * @returns The ID of this client
		 */
		client_id_t getID() const noexcept;
		/**
		 * @returns The path to this client's named pipe
		 */
		const std::filesystem::path &getPipePath() const noexcept;

		/**
		 * Checks whether the provided secret matches with the one provided by this client.
		 *
		 * @param secret The secret to verify
		 * @returns Whether the provided secret matches
		 */
		bool secretMatches(const std::string &secret) const noexcept;

		/**
		 * @returns Whether this client is currently in a valid state
		 */
		operator bool() const noexcept;
	};

}; // namespace JsonBridge
}; // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_BRIDGECLIENT_H_
