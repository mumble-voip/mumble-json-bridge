// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/messages/Message.h"

#include <boost/algorithm/string.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

		std::string to_string(MessageType type) {
			switch (type) {
				case MessageType::REGISTRATION:
					return "Registration";
				case MessageType::API_CALL:
					return "api_call";
				case MessageType::DISCONNECT:
					return "disconnect";
			}

			throw std::invalid_argument(std::string("Unknown message type \"")
										+ std::to_string(static_cast< int >(type)) + "\"");
		}

		MessageType type_from_string(const std::string &type) {
			if (boost::iequals(type, "registration")) {
				return MessageType::REGISTRATION;
			} else if (boost::iequals(type, "api_call")) {
				return MessageType::API_CALL;
			} else if (boost::iequals(type, "disconnect")) {
				return MessageType::DISCONNECT;
			} else {
				throw std::invalid_argument(std::string("Unknown message type \"") + type + "\"");
			}
		}

		MessageType parseBasicFormat(const nlohmann::json &msg) {
			if (!msg.is_object()) {
				throw InvalidMessageException("The given message is not a JSON object");
			}

			MESSAGE_ASSERT_FIELD(msg, "message_type", string);

			MessageType type;
			try {
				type = type_from_string(msg["message_type"].get< std::string >());
			} catch (const std::invalid_argument &) {
				throw InvalidMessageException(std::string("The given message_type \"")
											  + msg["message_type"].get< std::string >() + "\" is unknown");
			}

			if (type != MessageType::DISCONNECT) {
				// The disconnect message doesn't require a message body
				MESSAGE_ASSERT_FIELD(msg, "message", object);
			}

			return type;
		}

		Message::Message(MessageType type) : m_type(type) {}

		Message::~Message() {}

		MessageType Message::getType() const noexcept { return m_type; }
	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble
