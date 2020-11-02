// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "JSONInstruction.h"
#include "JSONInterface.h"
#include "handleOperation.h"

#include <mumble/json_bridge/messages/Message.h>

namespace Mumble {
namespace JsonBridge {
	namespace CLI {

		JSONInstruction::JSONInstruction(const nlohmann::json &msg) : m_msg(msg) {}

		nlohmann::json JSONInstruction::execute(const JSONInterface &jsonInterface) {
			MESSAGE_ASSERT_FIELD(m_msg, "message_type", string);

			if (m_msg["message_type"].get< std::string >() == "api_call") {
				return jsonInterface.process(m_msg);
			} else if (m_msg["message_type"].get< std::string >() == "operation") {
				return handleOperation(m_msg["message"],
									   [&jsonInterface](nlohmann::json &msg) { return jsonInterface.process(msg); });
			} else {
				throw Messages::InvalidMessageException(std::string("Unknown \"message_type\" option \"")
														+ m_msg["message_type"].get< std::string >() + "\"");
			}
		}

	}; // namespace CLI
};     // namespace JsonBridge
};     // namespace Mumble
