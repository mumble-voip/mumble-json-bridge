// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_MESSAGES_REGISTRATION_H_
#define MUMBLE_JSONBRIDGE_MESSAGES_REGISTRATION_H_

#include "mumble/json_bridge/messages/Message.h"

#include <string>

#include <nlohmann/json.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

		/**
		 * This class represents a message for registering a new client to the Mumble-JSON-Bridge
		 */
		class Registration : public Message {
		public:
			/**
			 * The extracted path to the client's named pipe
			 */
			std::string m_pipePath;
			/**
			 * The extracted secret the client has provided
			 */
			std::string m_secret;

			/**
			 * Parses the given message and populates the members of this instance accordingly. If the message
			 * doesn't fulfill the requirements, this constructor will throw an InvalidMessageException.
			 *
			 * @param msg The **body** of the registration message
			 */
			explicit Registration(const nlohmann::json &msg);
		};
	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_MESSAGES_REGISTRATION_H_
