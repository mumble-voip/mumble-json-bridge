# mumble-json-bridge

A Mumble plugin that offers a JSON API for Mumble interaction via named pipes.

This project consists of 3 (more or less) separate parts:
1. [The backend](json_bridge/)
2. [The plugin](plugin/)
3. [The CLI](cli/)

## Build dependencies

In order to build the project, you will require
- A cpp17 conform compiler
- CMake v3.10 or more recent
- Boost (required components: program-options and thread)
- Python3 with the [PyYAML](https://pypi.org/project/PyYAML/) package (only needed when building the CLI)
