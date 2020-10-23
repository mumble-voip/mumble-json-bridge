// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_CLI_INSTRUCTION_H_
#define MUMBLE_JSONBRIDGE_CLI_INSTRUCTION_H_

#include <nlohmann/json.hpp>

namespace Mumble {
namespace JsonBridge {
	namespace CLI {

		class JSONInterface;

		/**
		 * Abstract base class for all kinds of instructions that this CLI can process.
		 */
		class Instruction {
		public:
			/**
			 * Executes this instruction.
			 *
			 * @param jsonInterface The JSONInterface to use for API calls
			 * @returns The result of executing this instruction
			 */
			virtual nlohmann::json execute(const JSONInterface &jsonInterface) = 0;
		};

	}; // namespace CLI
};     // namespace JsonBridge
};     // namespace Mumble


#endif // MUMBLE_JSONBRIDGE_CLI_INSTRUCTION_H_

