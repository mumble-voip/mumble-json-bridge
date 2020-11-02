// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/plugin/MumblePlugin.h"
#include <mumble/json_bridge/Bridge.h>

#include <iostream>

class MumbleJsonBridge : public MumblePlugin {
private:
	Mumble::JsonBridge::Bridge m_bridge;

public:
	MumbleJsonBridge()
		: MumblePlugin("JSON Bridge", "Mumble Developers",
					   "This plugin offers a JSON API for Mumble interaction via named pipes"),
		  m_bridge(m_api) {}

	mumble_error_t init() noexcept override {
		std::cout << "JSON-Bridge initialized" << std::endl;

		m_bridge.start();

		return STATUS_OK;
	}

	void shutdown() noexcept override {
		m_bridge.stop(true);
		std::cout << "JSON-Bridge shut down" << std::endl;
	}

	void releaseResource(const void *ptr) noexcept override { std::terminate(); }
};

MumblePlugin &MumblePlugin::getPlugin() noexcept {
	static MumbleJsonBridge bridge;

	return bridge;
}
