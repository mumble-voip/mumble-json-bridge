// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/messages/APICall.h"

// define JSON serialization functions
template< typename ContentType > void to_json(nlohmann::json &j, const MumbleArray< ContentType > &array) {
	std::vector< ContentType > vec;
	vec.reserve(array.size());

	for (const auto &current : array) {
		vec.push_back(current);
	}

	j = vec;
}

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

// include function implementations
#include "APICall_handleImpl.cpp"

		APICall::APICall(const MumbleAPI &api, const nlohmann::json &msg)
			: Message(MessageType::API_CALL), m_api(api), m_msg(msg) {
			MESSAGE_ASSERT_FIELD(msg, "function", string);

			m_functionName = msg["function"].get< std::string >();

			if (s_allFunctions.count(m_functionName) == 0) {
				throw InvalidMessageException(std::string("Unknown API function \"") + m_functionName + "\"");
			}

			if (s_noParamFunctions.count(m_functionName) == 0) {
				MESSAGE_ASSERT_FIELD(msg, "parameter", object);
			}
		}

		nlohmann::json APICall::execute(const std::string &bridgeSecret) const {
			return ::Mumble::JsonBridge::Messages::execute(m_functionName, m_api, bridgeSecret, m_msg);
		}

	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble
