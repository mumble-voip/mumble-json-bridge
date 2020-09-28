// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/messages/Registration.h"

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

		Registration::Registration(const nlohmann::json &msg) : Message(MessageType::REGISTRATION) {
			MESSAGE_ASSERT_FIELD(msg, "pipe_path", string);
			MESSAGE_ASSERT_FIELD(msg, "secret", string);

			m_pipePath = msg["pipe_path"].get< std::string >();
			m_secret   = msg["secret"].get< std::string >();
		}

	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble
