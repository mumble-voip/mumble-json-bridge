// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_MESSAGES_MESSAGE_H_
#define MUMBLE_JSONBRIDGE_MESSAGES_MESSAGE_H_

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#define MESSAGE_ASSERT_FIELD(msg, name, type)                                                                        \
	if (!msg.contains(name)) {                                                                                       \
		throw ::Mumble::JsonBridge::Messages::InvalidMessageException("The given message does not specify a \"" name \
																	  "\" field");                                   \
	}                                                                                                                \
	if (!msg[name].is_##type()) {                                                                                    \
		throw ::Mumble::JsonBridge::Messages::InvalidMessageException("The \"" name                                  \
																	  "\" field is expected to be of type " #type);  \
	}

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

		/**
		 * An exception thrown if a message doesn't fulfill the requirements
		 */
		class InvalidMessageException : public std::logic_error {
		public:
			using std::logic_error::logic_error;
		};

		/**
		 * An enum holding the possible message types
		 */
		enum class MessageType { REGISTRATION, API_CALL, DISCONNECT };

		/**
		 * @return A unique string representation of the give MessageType. If no such
		 * representation can be found, an exception is thrown.
		 *
		 * @param type The type to convert to string
		 *
		 * @see Mumble::JsonBridge::Messages::MessageType
		 */
		std::string to_string(MessageType type);
		/**
		 * @return The MessageType corresponding to the provided string representation. If the provided string
		 * is not a valid representation of a MessageType, this function will throw an exception.
		 *
		 * @param type The type's string representation
		 *
		 * @see Mumble::JsonBridge::Messages::MessageType
		 */
		MessageType type_from_string(const std::string &type);

		/**
		 * Verifies that the message represented by the given JSON object fulfills the basic requirements
		 * of a message sent to the Mumble-JSON-Bridge.
		 *
		 * @param msg The JSON representation of the message
		 * @returns The MessageType of the provided message
		 *
		 * @see Mumble::JsonBridge::Messages::MessageType
		 */
		MessageType parseBasicFormat(const nlohmann::json &msg);

		/**
		 * This class represents a message received by the Mumble-JSON-Bridge
		 */
		class Message {
		protected:
			/**
			 * The type of this message
			 */
			MessageType m_type;

			Message(MessageType type);

		public:
			virtual ~Message();

			/**
			 * @return The MessageType of this message
			 *
			 * @see Mumble::JsonBridge::Messages::MessageType
			 */
			MessageType getType() const noexcept;
		};

	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_MESSAGES_MESSAGE_H_
