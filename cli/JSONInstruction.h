// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_CLI_JSONINSTRUCTION_H_
#define MUMBLE_JSONBRIDGE_CLI_JSONINSTRUCTION_H_

#include "Instruction.h"

#include <nlohmann/json.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace CLI {

		/**
		 * An instruction that is received in JSON format
		 */
		class JSONInstruction : public Instruction {
		private:
			/**
			 * The original JSON message
			 */
			nlohmann::json m_msg;

		public:
			JSONInstruction(const nlohmann::json &msg);
			virtual nlohmann::json execute(const JSONInterface &jsonInterface) override;
		};

	}; // namespace CLI
};     // namespace JsonBridge
};     // namespace Mumble


#endif // MUMBLE_JSONBRIDGE_CLI_JSONINSTRUCTION_H_

