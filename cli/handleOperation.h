// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_CLI_HANDLEOPERATION_H_
#define MUMBLE_JSONBRIDGE_CLI_HANDLEOPERATION_H_

#include <nlohmann/json.hpp>

#include <functional>
#include <stdexcept>

/**
 * Exception that is being thrown if the execution of an operation fails.
 */
class OperationException : public std::logic_error {
public:
	using std::logic_error::logic_error;
};

/**
 * Process a message that requests the execution of an operation (message_type == "operation").
 *
 * @param msg The JSON representation of the respective message
 * @param executeQuery A functor that can be used to send API calls to the JSON bridge
 * @returns The result of the operation (JSON format). This will be the JSON response to
 * the last performed API call.
 */
nlohmann::json handleOperation(const nlohmann::json &msg,
							   const std::function< nlohmann::json(nlohmann::json &) > &executeQuery);

#endif // MUMBLE_JSONBRIDGE_CLI_HANDLEOPERATION_H_
