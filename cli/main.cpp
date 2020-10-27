// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "Instruction.h"
#include "JSONInstruction.h"
#include "JSONInterface.h"
#include "handleOperation.h"

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
	try {
		boost::program_options::options_description desc("Command-line interface for the Mumble-JSON-Bridge");

		uint32_t readTimeout;
		uint32_t writeTimeout;

		desc.add_options()("help,h", "Produces this help message")("json,j",
																   boost::program_options::value< std::string >(),
																   "Specifies the JSON message to be sent to Mumble")(
			"read-timeout,r", boost::program_options::value< uint32_t >(&readTimeout)->default_value(1000),
			"The timeout for read-operations (in ms)")(
			"write-timeout,w", boost::program_options::value< uint32_t >(&writeTimeout)->default_value(100),
			"The timeout for write-operations (in ms)");

		boost::program_options::variables_map vm;
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
		boost::program_options::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			return 0;
		}

		nlohmann::json json;
		if (vm.count("json")) {
			json = nlohmann::json::parse(vm["json"].as< std::string >());
		} else {
			// Read all content from stdin
			std::string content(std::istreambuf_iterator< char >(std::cin), {});
			boost::trim(content);

			json = nlohmann::json::parse(content);
		}

		Mumble::JsonBridge::CLI::JSONInstruction instruction(json);

		Mumble::JsonBridge::CLI::JSONInterface jsonInterface(readTimeout, writeTimeout);

		std::cout << instruction.execute(jsonInterface).dump(2) << std::endl;
	} catch (const Mumble::JsonBridge::TimeoutException &) {
		std::cerr << "[ERROR]: The operation timed out (Are you sure the JSON Bridge is running?)" << std::endl;
		return 2;
	} catch (const OperationException &e) {
		std::cerr << "[ERROR]: Operation failed: " << e.what() << std::endl;
		return 3;
	} catch (const std::exception &e) {
		std::cerr << "[ERROR]: " << e.what() << std::endl;
		return 4;
	}

	return 0;
}
