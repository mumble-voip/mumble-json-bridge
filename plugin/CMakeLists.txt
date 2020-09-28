# Copyright 2020 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# source tree.

add_library(json_bridge_plugin
	SHARED
		plugin.cpp
)

target_link_libraries(json_bridge_plugin PUBLIC json_bridge mumble_plugin_cpp_wrapper)
