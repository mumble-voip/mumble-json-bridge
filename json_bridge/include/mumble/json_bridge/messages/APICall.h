// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_MESSAGES_APICALL_H_
#define MUMBLE_JSONBRIDGE_MESSAGES_APICALL_H_

#include "mumble/json_bridge/messages/Message.h"

#include <mumble/plugin/MumbleAPI.h>

#include <string>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace Messages {

		/**
		 * This class represents a message that requests the Bridge to call a specific Mumble API function
		 */
		class APICall : public Message {
		private:
			/**
			 * The name of the API function that should be called
			 */
			std::string m_functionName;
			/**
			 * A reference to a MumbleAPI
			 */
			const MumbleAPI &m_api;
			/**
			 * The **body** of the API-call request message
			 */
			nlohmann::json m_msg;

			/**
			 * A set of all available API function names
			 */
			static const std::unordered_set< std::string > s_allFunctions;
			/**
			 * A set of the names of all API functions that don't take any parameter
			 */
			static const std::unordered_set< std::string > s_noParamFunctions;

		public:
			/**
			 * Creates an instance of this Message. If the provided message doesn't fulfill
			 * the requirements of this class, this constructor will throw an InvalidMessageException.
			 *
			 * @param api A **reference** to a MumbleAPI. The lifetime of this API must not be shorter than the one of
			 * this instance
			 * @param msg The **body** of the API-call request message
			 */
			explicit APICall(const MumbleAPI &api, const nlohmann::json &msg);

			/**
			 * Executes the requested API function
			 *
			 * @returns JSON representation of the message describing the status of the invocation (including potential
			 * return values)
			 */
			nlohmann::json execute(const std::string &bridgeSecret) const;
		};
	}; // namespace Messages
};     // namespace JsonBridge
};     // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_MESSAGES_APICALL_H_
