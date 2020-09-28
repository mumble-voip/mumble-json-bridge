// Copyright 2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// source tree.

#ifndef MUMBLE_JSONBRIDGE_BRIDGE_H_
#define MUMBLE_JSONBRIDGE_BRIDGE_H_

#include "mumble/json_bridge/BridgeClient.h"
#include "mumble/json_bridge/NamedPipe.h"

#include "mumble/json_bridge/messages/APICall.h"
#include "mumble/json_bridge/messages/Registration.h"

#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/thread/thread.hpp>

#include <nlohmann/json.hpp>

#include <mumble/plugin/MumbleAPI.h>

namespace Mumble {
namespace JsonBridge {

	/**
	 * Tbis class represents the heart of the Mumble-JSON-Bridge. It is responsible for creating a new thread in which it'll create
	 * the named pipe used for communication. It will enter a loop of polling for messages until the bridge is stopped again.
	 */
	class Bridge {
	private:
		/**
		 * Little helper mutex that is needed in order to ensure that m_workerThread has been assigned properly before being
		 * accessed for the first time in the new thread.
		 */
		std::mutex m_startMutex;
		/**
		 * The worker thread of this class in which basically all operations of this class happen.
		 */
		boost::thread m_workerThread;
		/**
		 * The pipe-instance that is used for communication
		 */
		NamedPipe m_pipe;
		/**
		 * A map of currently registered clients
		 */
		std::unordered_map< client_id_t, BridgeClient > m_clients;
		/**
		 * The bridge's secret used to identify itself when talking to clients
		 */
		std::string m_secret;
		/**
		 * A **reference** to the MumbleAPI. API-call requests will be forwarded to and processed by it.
		 */
		const MumbleAPI &m_api;

		/**
		 * A continuous counter for assigning unique IDs to new clients. This variable must not be accessed
		 * outside of m_workerThread.
		 *
		 * @see Mumble::JsonBridge::Bridge::m_workerThread
		 */
		static client_id_t s_nextClientID;

		/**
		 * Internal start-method that must not be called outside of m_workerThread
		 *
		 * @see Mumble::JsonBridge::Bridge::m_workerThread
		 */
		void doStart();
		/**
		 * Method used to process received messages
		 *
		 * @msg The JSON representation of the respective message
		 */
		void processMessage(const nlohmann::json &msg);

		/**
		 * Used to handle registration messages.
		 *
		 * @param msg The message to process
		 */
		void handleRegistration(const Messages::Registration &msg);
		/**
		 * Used to handle API-call request messages.
		 *
		 * @param msg The message to process
		 */
		void handleAPICall(const BridgeClient &client, const Messages::APICall &msg) const;
		/**
		 * Used to handle disconnect messages
		 *
		 * @param msg The JSON representation of the message to process
		 */
		void handleDisconnect(const nlohmann::json &msg);

	public:
		/**
		 * The path at which the Bridge's named pipe will be made available. If it doesn't exist, this means that
		 * the Bridge has not started (yet).
		 */
		static const std::filesystem::path s_pipePath;

		/**
		 * Creates a new instance of this Bridge
		 *
		 * @param api A **reference** to the MumbleAPI. The lifetime of the referenced API object must not be shorter than the lifetime
		 * of this Bridge object!
		 */
		Bridge(const MumbleAPI &api);

		/**
		 * Start the bridge. Note that this will be done in a different thread, so this function is will return immediately.
		 */
		void start();
		/**
		 * Stops the bridge.
		 *
		 * @param join Whether this function should wait on the worker-thread to terminate. Otherwise this function will
		 * return immediately.
		 */
		void stop(bool join);
	};

}; // namespace JsonBridge
}; // namespace Mumble

#endif // MUMBLE_JSONBRIDGE_BRIDGE_H_
