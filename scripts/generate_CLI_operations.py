#!/usr/bin/env python3
#
# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

import yaml
import argparse
import os
from datetime import datetime

def generateLicenseHeader():
    currentYear = datetime.today().strftime("%Y")

    licenseHeader = "// Copyright " + (currentYear + "-" if not currentYear == "2020" else "") +  "2020 The Mumble Developers. All rights reserved.\n"
    licenseHeader += "// Use of this source code is governed by a BSD-style license\n"
    licenseHeader += "// that can be found in the LICENSE file at the root of the\n"
    licenseHeader += "// source tree.\n"

    return licenseHeader


def jsonTypeToCppType(jsonType):
    if jsonType == "string":
        return "std::string"
    elif jsonType == "number":
        return "double"
    elif jsonType == "number_integer":
        return "int"
    elif jsonType == "number_unsigned":
        return "uint64_t"
    elif jsonType == "number_float":
        return "double"
    elif jsonType == "boolean":
        return "bool"

    raise RuntimeError("Unable to convert \"%s\" from JSON to cpp" % jsonType)


def generateAPIResponseCheck():
    generatedCode = "void checkAPIResponse(const nlohmann::json &response) {\n"
    generatedCode += "\tif (response.contains(\"response_type\") && response[\"response_type\"].get<std::string>() == \"api_call\") {\n"
    generatedCode += "\t\t// All good\n"
    generatedCode += "\t\treturn;\n"
    generatedCode += "\t}\n"
    generatedCode += "\t\n"
    generatedCode += "\t// There seems to have been an error\n"
    generatedCode += "\tif (!response.contains(\"response_type\") || !response.contains(\"response\")) {\n"
    generatedCode += "\t\t// We can't process this response - seems invalid\n"
    generatedCode += "\t\tthrow OperationException(\"Got invalid response from Mumble-JSON-Bridge.\");\n"
    generatedCode += "\t}\n"
    generatedCode += "\t\n"
    generatedCode += "\tif (response[\"response_type\"].get<std::string>() == \"api_error\"\n"
    generatedCode += "\t\t|| response[\"response_type\"].get<std::string>() == \"api_error_optional\"\n"
    generatedCode += "\t\t|| response[\"response_type\"].get<std::string>() == \"error\") {\n"
    generatedCode += "\t\tthrow OperationException(response[\"response\"][\"error_message\"].get<std::string>());\n"
    generatedCode += "\t} else {\n"
    generatedCode += "\t\tthrow OperationException(\"Generic API error ecountered\");\n"
    generatedCode += "\t}\n"
    generatedCode += "}\n"

    return generatedCode
     


def generateParameterProcessing(parameter, messageName, operationName):
    generatedCode = "// Validate and extract parameter\n"
    generatedCode += "\n"

    # verify that the correct amount of parameter is provided
    generatedCode += "MESSAGE_ASSERT_FIELD(" + messageName + ", \"parameter\", object);\n"
    generatedCode += "\n"
    generatedCode += "const nlohmann::json &operationParams = " + messageName + "[\"parameter\"];\n"
    generatedCode += "\n"
    generatedCode += "if (operationParams.size() != " + str(len(parameter)) + ") {\n"
    generatedCode += "\tthrow ::Mumble::JsonBridge::Messages::InvalidMessageException(std::string(\"Operation \\\"" + operationName \
            + "\\\" expects " + str(len(parameter)) + " parameter, but was provided with \") + std::to_string(operationParams.size()));\n"
    generatedCode += "}\n"
    generatedCode += "\n"

    # Check and extract actual parameter
    for currentParam in parameter:
        paramName = currentParam["name"]
        paramType = currentParam["type"]

        generatedCode += "MESSAGE_ASSERT_FIELD(operationParams, \"" + paramName + "\", " + paramType + ");\n"
        generatedCode += jsonTypeToCppType(paramType) + " " + paramName + " = operationParams[\"" + paramName + "\"].get<"\
                + jsonTypeToCppType(paramType) + ">();\n"
        generatedCode += "\n"

    return generatedCode


def generateAPIFunctionCall(queryName, responseName, functionName, parameter):
    generatedCode = "// clang-format off\n"
    generatedCode += "nlohmann::json " + queryName + " = {\n"
    generatedCode += "\t{ \"message_type\", \"api_call\" },\n"
    generatedCode += "\t{ \"message\",\n"
    generatedCode += "\t\t{\n"
    generatedCode += "\t\t\t{ \"function\", \"" + functionName + "\" }"

    if len(parameter) > 0:
        generatedCode += ",\n"
        generatedCode += "\t\t\t{ \"parameter\",\n"
        generatedCode += "\t\t\t\t{\n"

        for currentParam in parameter:
            value = currentParam["value"]
            if not value:
                # convert empty to empty string
                value  = "\"\""

            if type(value) == bool:
                # Make sure that booleans are used in a c-compatible way
                value = "true" if value else "false"

            assert type(value) == type("")
            generatedCode += "\t\t\t\t\t{ \"" + currentParam["name"] + "\", " + value + " },\n"

        # remove trailing ",\n"
        generatedCode = generatedCode[ : -2]
        generatedCode += "\n"
        generatedCode += "\t\t\t\t}\n"

        generatedCode += "\t\t\t}\n"
    else:
        generatedCode += "\n"

    generatedCode += "\t\t}\n"
    generatedCode += "\t}\n"
    generatedCode += "};\n"
    generatedCode += "// clang-format on\n"
    generatedCode += "\n"
    generatedCode += "nlohmann::json " + responseName + " = executeQuery(" + queryName + ");\n"

    return generatedCode


def generateFunctionAssignment(varName, varType, functionSpec, counter):
    generatedCode = ""

    if functionSpec["type"] == "api":
        generatedCode += generateAPIFunctionCall("query" + str(counter), "response" + str(counter), functionSpec["name"], \
                functionSpec["parameter"] if "parameter" in functionSpec else [])
        generatedCode += "\n"
        generatedCode += "checkAPIResponse(response" + str(counter) + ");\n"
        generatedCode += jsonTypeToCppType(varType) + " " + varName + " = response" + str(counter) + "[\"response\"][\"return_value\"].get<" \
                + jsonTypeToCppType(varType) + ">();\n"
    elif functionSpec["type"] == "regular":
        generatedCode += jsonTypeToCppType(varType) + " " + varName + " = " + functionSpec["name"] + "("

        if "parameter" in functionSpec:
            for currentParam in functionSpec["parameter"]:
                generatedCode += currentParam["value"] + ", "

            # Remove trailing ", "
            generatedCode = generatedCode[ : -2]

            generatedCode += ");\n"
    else:
        raise RuntimeError("Unknown function type %s" % functionSpec["type"])

    return generatedCode


def generateDepencyProcessing(dependencies, operationName):
    generatedCode = "// Obtain all needed values\n"
    generatedCode += "\n"

    # Handle dependencies one at a time
    counter = 1
    for currentDep in dependencies:
        depName = currentDep["name"]
        depType = currentDep["type"]

        generatedCode += generateFunctionAssignment(depName, depType, currentDep["function"], counter).strip() + "\n"
        generatedCode += "\n"
        counter += 1

    return generatedCode


def generateExecuteStatement(execSpec):
    if not "function" in execSpec:
        raise RuntimeError("Missing \"function\" spec in \"executes\" statement")

    functionSpec = execSpec["function"]

    if not functionSpec["type"] == "api":
        raise RuntimeError("Currently only API function calls are supported for operation-executes statements")

    generatedCode = generateAPIFunctionCall("operationQuery", "result", \
            functionSpec["name"], functionSpec["parameter"] if "parameter" in functionSpec else []).strip() + "\n"
    generatedCode += "\n"
    generatedCode += "return result;"

    return generatedCode


def generatedDelegateFunction(operations):
    generatedCode = "nlohmann::json handleOperation(const nlohmann::json &msg,\n"
    generatedCode += "\tconst std::function<nlohmann::json(nlohmann::json &)> &executeQuery) {\n"
    generatedCode += "\tif (!msg.contains(\"operation\") || !msg[\"operation\"].is_string()) {\n"
    generatedCode += "\t\tthrow OperationException(\"Missing \\\"operation\\\" field (required to be of type string)\");\n"
    generatedCode += "\t}\n"
    generatedCode += "\n"

    for i in range(len(operations)):
        if i == 0:
            generatedCode += "\tif "
        else:
            generatedCode += " else if "
        
        generatedCode += "(msg[\"operation\"].get<std::string>() == \"" + operations[i] + "\") {\n" 
        generatedCode += "\t\treturn handle_" + operations[i] + "_operation(msg, executeQuery);\n"
        generatedCode += "\t}"

    generatedCode += " else {\n"
    generatedCode += "\t\tthrow OperationException(std::string(\"Unknown operation \\\"\") + msg[\"operation\"].get<std::string>() + \"\\\"\");\n"
    generatedCode += "\t}\n"
    generatedCode += "}\n"

    return generatedCode


def main():
    parser = argparse.ArgumentParser(description="Generates the implementation for the handle_*_operation functions")
    parser.add_argument("-i", "--input-dir", help="The path to directory containing the operation YAML files")
    parser.add_argument("-o", "--output-file", help="Path to which the generated source code shall be written")

    args = parser.parse_args()

    generatedCode = generateLicenseHeader()
    generatedCode += "\n"
    generatedCode += "// This file was auto-generated by scripts/generate_CLI_operations.py. DO NOT EDIT MANUALLY!\n"
    generatedCode += "\n"

    generatedCode += "#include \"handleOperation.h\"\n"
    generatedCode += "\n"
    generatedCode += "#include <mumble/json_bridge/messages/Message.h>\n"
    generatedCode += "\n"

    generatedCode += generateAPIResponseCheck()
    generatedCode += "\n"

    operations = []

    if args.input_dir is None:
        print("[ERROR]: No input-dir given")
        return

    for currentFile in os.listdir(args.input_dir):
        fileName = os.fsdecode(currentFile)

        if not fileName.endswith(".yaml"):
            print("Skipping \"%s\"" % fileName)
            continue

        with open(os.path.join(args.input_dir, fileName), "r") as definitionFile:
            documents = yaml.full_load(definitionFile)

            for currentOp in documents["operations"]:
                operationName = currentOp["operation"]
                operations.append(operationName)

                generatedCode += "nlohmann::json handle_" + operationName + "_operation(const nlohmann::json &msg,\n"
                generatedCode += "\tconst std::function<nlohmann::json(nlohmann::json &)> &executeQuery) {\n"

                if "parameter" in currentOp:
                    # Handle the parameter that the JSON message might contain
                    generatedCode += "\t" + generateParameterProcessing(currentOp["parameter"], "msg", operationName).replace("\n", "\n\t").strip() + "\n"
                else:
                    # make sure there are no parameter given as none are expected
                    generatedCode += "\tif (msg.contains(\"parameter\")) {\n"
                    generatedCode += "\t\t::Mumble::JsonBridge::Messages::InvalidMessageException(\"Operation \\\"" + operationName \
                            + "\\\" does not take any parameter\");\n"
                    generatedCode += "\t}\n"

                generatedCode += "\n"

                if "depends" in currentOp:
                    generatedCode += "\t" + generateDepencyProcessing(currentOp["depends"], operationName).replace("\n", "\n\t").strip() + "\n"

                generatedCode += "\n"
                generatedCode += "\n"

                generatedCode += "\t// Now actually execute the operation\n"
                generatedCode += "\t" + generateExecuteStatement(currentOp["executes"]).replace("\n", "\n\t").strip() + "\n"
                
                generatedCode += "}\n"
                generatedCode += "\n"

    # Generate function for delegating
    generatedCode += generatedDelegateFunction(operations)


    if not args.output_file is None:
        outFile = open(args.output_file, "w")
        outFile.write(generatedCode)
    else:
        print(generatedCode)


if __name__ == "__main__":
    main()
