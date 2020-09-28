// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#include "mumble/json_bridge/BridgeClient.h"
#include "mumble/json_bridge/NamedPipe.h"

namespace Mumble {
namespace JsonBridge {
	BridgeClient::BridgeClient(const std::filesystem::path &pipePath, const std::string &secret, client_id_t id)
		: m_pipePath(pipePath), m_secret(secret), m_id(id) {}

	BridgeClient::~BridgeClient() {}

	void BridgeClient::write(const std::string &message) const { NamedPipe::write(m_pipePath, message); }

	client_id_t BridgeClient::getID() const noexcept { return m_id; }

	const std::filesystem::path &BridgeClient::getPipePath() const noexcept { return m_pipePath; }

	bool BridgeClient::secretMatches(const std::string &secret) const noexcept { return m_secret == secret; }

	BridgeClient::operator bool() const noexcept { return m_id != INVALID_CLIENT_ID; }
}; // namespace JsonBridge
}; // namespace Mumble
