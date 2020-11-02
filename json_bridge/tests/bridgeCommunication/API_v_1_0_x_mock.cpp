// Copyright 2019-2020 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "API_v_1_0_x_mock.h"

#include <mumble/plugin/internal/PluginComponents_v_1_0_x.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <vector>


// Pretend to only know a plugin of ID 42
#define VERIFY_PLUGIN_ID(id)         \
	if (id != pluginID) {            \
		return EC_INVALID_PLUGIN_ID; \
	}
// Pretend to only know a connection of ID 13
#define VERIFY_CONNECTION(connection)    \
	if (connection != activeConnetion) { \
		return EC_CONNECTION_NOT_FOUND;  \
	}
// Pretend that the connection is always synchronized
#define ENSURE_CONNECTION_SYNCHRONIZED(connection) \
	if (false) {                                   \
		return EC_CONNECTION_UNSYNCHRONIZED;       \
	}

#define UNUSED(var) (void) var

namespace API_Mock {

std::unordered_map< std::string, int > calledFunctions;

/// A "curator" that will keep track of allocated resources and how to delete them
struct MumbleAPICurator {
	std::vector< const void * > m_allocatedMemory;

	~MumbleAPICurator() {
		// free all allocated functions

		if (m_allocatedMemory.size() > 0) {
			std::cerr << "There are " << m_allocatedMemory.size() << " leaking resources" << std::endl;
			std::exit(1);
		}
	}
};

static MumbleAPICurator curator;


//////////////////////////////////////////////
/////////// API IMPLEMENTATION ///////////////
//////////////////////////////////////////////

// The description of the functions is provided in MumbleAPI.h

mumble_error_t PLUGIN_CALLING_CONVENTION freeMemory_v_1_0_x(mumble_plugin_id_t callerID, const void *ptr) {
	calledFunctions["freeMemory"]++;

	// Don't verify plugin ID here to avoid memory leaks
	UNUSED(callerID);

	auto it = std::find(curator.m_allocatedMemory.begin(), curator.m_allocatedMemory.end(), ptr);

	if (it == curator.m_allocatedMemory.end()) {
		return EC_POINTER_NOT_FOUND;
	} else {
		curator.m_allocatedMemory.erase(it);

		free(const_cast< void * >(ptr));

		return STATUS_OK;
	}
}

mumble_error_t PLUGIN_CALLING_CONVENTION getActiveServerConnection_v_1_0_x(mumble_plugin_id_t callerID,
																		   mumble_connection_t *connection) {
	calledFunctions["getActiveServerConnection"]++;

	VERIFY_PLUGIN_ID(callerID);

	*connection = 13;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION isConnectionSynchronized_v_1_0_x(mumble_plugin_id_t callerID,
																		  mumble_connection_t connection,
																		  bool *synchronized) {
	calledFunctions["isConnectionSychronized"]++;

	VERIFY_PLUGIN_ID(callerID);
	VERIFY_CONNECTION(connection);

	// In this mock the connection is always assumed to be synchronized
	*synchronized = true;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getLocalUserID_v_1_0_x(mumble_plugin_id_t callerID,
																mumble_connection_t connection,
																mumble_userid_t *userID) {
	calledFunctions["getLocalUserID"]++;

	VERIFY_PLUGIN_ID(callerID);

	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	*userID = localUserID;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getUserName_v_1_0_x(mumble_plugin_id_t callerID,
															 mumble_connection_t connection, mumble_userid_t userID,
															 const char **name) {
	calledFunctions["getUserName"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	std::string userName;
	switch (userID) {
		case localUserID:
			userName = localUserName;
			break;
		case otherUserID:
			userName = otherUserName;
			break;
		default:
			return EC_USER_NOT_FOUND;
	}

	size_t size = userName.size() + 1;

	char *nameArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(nameArray, userName.c_str());

	curator.m_allocatedMemory.push_back(nameArray);

	*name = nameArray;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getChannelName_v_1_0_x(mumble_plugin_id_t callerID,
																mumble_connection_t connection,
																mumble_channelid_t channelID, const char **name) {
	calledFunctions["getChannelName"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	std::string channelName;
	switch (channelID) {
		case localUserChannel:
			channelName = localUserChannelName;
			break;
		case otherUserChannel:
			channelName = otherUserChannelName;
			break;
		default:
			return EC_CHANNEL_NOT_FOUND;
	}

	size_t size = channelName.size() + 1;

	char *nameArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(nameArray, channelName.c_str());

	curator.m_allocatedMemory.push_back(nameArray);

	*name = nameArray;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getAllUsers_v_1_0_x(mumble_plugin_id_t callerID,
															 mumble_connection_t connection, mumble_userid_t **users,
															 size_t *userCount) {
	calledFunctions["getAllUsers"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	size_t amount = 2;

	mumble_userid_t *userIDs = reinterpret_cast< mumble_userid_t * >(malloc(sizeof(mumble_userid_t) * amount));

	userIDs[0] = localUserID;
	userIDs[1] = otherUserID;

	curator.m_allocatedMemory.push_back(userIDs);

	*users     = userIDs;
	*userCount = amount;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getAllChannels_v_1_0_x(mumble_plugin_id_t callerID,
																mumble_connection_t connection,
																mumble_channelid_t **channels, size_t *channelCount) {
	calledFunctions["getAllChannels"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	size_t amount = 2;

	mumble_channelid_t *channelIDs =
		reinterpret_cast< mumble_channelid_t * >(malloc(sizeof(mumble_channelid_t) * amount));

	channelIDs[0] = localUserChannel;
	channelIDs[1] = otherUserChannel;

	curator.m_allocatedMemory.push_back(channelIDs);

	*channels     = channelIDs;
	*channelCount = amount;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getChannelOfUser_v_1_0_x(mumble_plugin_id_t callerID,
																  mumble_connection_t connection,
																  mumble_userid_t userID, mumble_channelid_t *channel) {
	calledFunctions["getChannelOfUser"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	switch (userID) {
		case localUserID:
			*channel = localUserChannel;
			break;
		case otherUserID:
			*channel = otherUserChannel;
			break;
		default:
			return EC_USER_NOT_FOUND;
	}

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getUsersInChannel_v_1_0_x(mumble_plugin_id_t callerID,
																   mumble_connection_t connection,
																   mumble_channelid_t channelID,
																   mumble_userid_t **userList, size_t *userCount) {
	calledFunctions["getUsersInChannel"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (channelID != localUserChannel && channelID != otherUserChannel) {
		return EC_CHANNEL_NOT_FOUND;
	}

	size_t amount = 1;

	mumble_userid_t *userIDs = reinterpret_cast< mumble_userid_t * >(malloc(sizeof(mumble_userid_t) * amount));

	if (channelID == localUserChannel) {
		userIDs[0] = localUserID;
	} else {
		userIDs[0] = otherUserID;
	}

	curator.m_allocatedMemory.push_back(userIDs);

	*userList  = userIDs;
	*userCount = amount;

	return STATUS_OK;
}


mumble_error_t PLUGIN_CALLING_CONVENTION
	getLocalUserTransmissionMode_v_1_0_x(mumble_plugin_id_t callerID, mumble_transmission_mode_t *transmissionMode) {
	calledFunctions["getLocalUserTransmissionMode"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Pretend local user always uses VAD
	*transmissionMode = TM_VOICE_ACTIVATION;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION isUserLocallyMuted_v_1_0_x(mumble_plugin_id_t callerID,
																	mumble_connection_t connection,
																	mumble_userid_t userID, bool *muted) {
	calledFunctions["isUserLocallyMuted"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (userID != localUserID && userID != otherUserID) {
		return EC_USER_NOT_FOUND;
	}

	// pretend the other user is locally muted
	*muted = userID == otherUserID;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION isLocalUserMuted_v_1_0_x(mumble_plugin_id_t callerID,
																  mumble_connection_t connection, bool *muted) {
	calledFunctions["isLocalUserMuted"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	*muted = false;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION isLocalUserDeafened_v_1_0_x(mumble_plugin_id_t callerID,
																	 mumble_connection_t connection, bool *deafened) {
	calledFunctions["isLocalUserDeafened"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	*deafened = false;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getUserHash_v_1_0_x(mumble_plugin_id_t callerID,
															 mumble_connection_t connection, mumble_userid_t userID,
															 const char **hash) {
	calledFunctions["getUserHash"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	std::string userHash;
	switch (userID) {
		case localUserID:
			userHash = "85240b5b2d5ef4227270d2a400957140d2299523";
			break;
		case otherUserID:
			userHash = "4535efde23c002a726072c9c39d9ede9d3e76be5";
			break;
		default:
			return EC_USER_NOT_FOUND;
	}

	// The user's hash is already in hexadecimal representation, so we don't have to worry about null-bytes in it
	size_t size = userHash.size() + 1;

	char *hashArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(hashArray, userHash.c_str());

	curator.m_allocatedMemory.push_back(hashArray);

	*hash = hashArray;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getServerHash_v_1_0_x(mumble_plugin_id_t callerID,
															   mumble_connection_t connection, const char **hash) {
	calledFunctions["getServerHash"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	std::string strHash = "9449d173bcc01d96c6a01de5b93f0d70760fb0f2";

	size_t size = strHash.size() + 1;

	char *hashArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(hashArray, strHash.c_str());

	curator.m_allocatedMemory.push_back(hashArray);

	*hash = hashArray;

	return STATUS_OK;
}


mumble_error_t PLUGIN_CALLING_CONVENTION
	requestLocalUserTransmissionMode_v_1_0_x(mumble_plugin_id_t callerID, mumble_transmission_mode_t transmissionMode) {
	calledFunctions["requestLocalUserTransmissionMode"]++;

	VERIFY_PLUGIN_ID(callerID);

	// We don't actually set the transmission mode

	switch (transmissionMode) {
		case TM_CONTINOUS:
			break;
		case TM_VOICE_ACTIVATION:
			break;
		case TM_PUSH_TO_TALK:
			break;
		default:
			return EC_UNKNOWN_TRANSMISSION_MODE;
	}

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getUserComment_v_1_0_x(mumble_plugin_id_t callerID,
																mumble_connection_t connection, mumble_userid_t userID,
																const char **comment) {
	calledFunctions["getUserComment"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (userID != localUserID && userID != otherUserID) {
		return EC_USER_NOT_FOUND;
	}

	std::string strComment = userID == localUserID ? "I am the local user" : "I am another user";

	size_t size = strComment.size() + 1;

	char *nameArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(nameArray, strComment.c_str());

	curator.m_allocatedMemory.push_back(nameArray);

	*comment = nameArray;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getChannelDescription_v_1_0_x(mumble_plugin_id_t callerID,
																	   mumble_connection_t connection,
																	   mumble_channelid_t channelID,
																	   const char **description) {
	calledFunctions["getChannelDescription"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (channelID != localUserChannel && channelID != otherUserChannel) {
		return EC_CHANNEL_NOT_FOUND;
	}

	std::string desc = channelID == localUserChannel ? localUserChannelDesc : otherUserChannelDesc;

	size_t size = desc.size() + 1;

	char *nameArray = reinterpret_cast< char * >(malloc(size * sizeof(char)));

	std::strcpy(nameArray, desc.c_str());

	curator.m_allocatedMemory.push_back(nameArray);

	*description = nameArray;

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestUserMove_v_1_0_x(mumble_plugin_id_t callerID,
																 mumble_connection_t connection, mumble_userid_t userID,
																 mumble_channelid_t channelID, const char *password) {
	calledFunctions["requestUserMove"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (userID != localUserID && userID != otherUserID) {
		return EC_USER_NOT_FOUND;
	}

	if (channelID != localUserChannel && channelID != otherUserChannel) {
		return EC_CHANNEL_NOT_FOUND;
	}

	// Don't actually move the user

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestMicrophoneActivationOverwrite_v_1_0_x(mumble_plugin_id_t callerID,
																					  bool activate) {
	calledFunctions["requestMicrophoneActivationOverwrite"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Don't actually do something
	UNUSED(activate);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestLocalMute_v_1_0_x(mumble_plugin_id_t callerID,
																  mumble_connection_t connection,
																  mumble_userid_t userID, bool muted) {
	calledFunctions["requestLocalMute"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (userID == localUserID) {
		// Can't locally mute the local user
		return EC_INVALID_MUTE_TARGET;
	}

	// Don't actually do something
	UNUSED(muted);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestLocalUserMute_v_1_0_x(mumble_plugin_id_t callerID,
																	  mumble_connection_t connection, bool muted) {
	calledFunctions["requestLocalUserMute"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	// Don't actually do something
	UNUSED(muted);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestLocalUserDeaf_v_1_0_x(mumble_plugin_id_t callerID,
																	  mumble_connection_t connection, bool deafened) {
	calledFunctions["requestLocalUserDeaf"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	// Don't actually do something
	UNUSED(deafened);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION requestSetLocalUserComment_v_1_0_x(mumble_plugin_id_t callerID,
																			mumble_connection_t connection,
																			const char *comment) {
	calledFunctions["requestSetLocalUserComment"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	UNUSED(comment);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION findUserByName_v_1_0_x(mumble_plugin_id_t callerID,
																mumble_connection_t connection, const char *userName,
																mumble_userid_t *userID) {
	calledFunctions["findUserByName"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (localUserName == userName) {
		std::cout << "Found user" << std::endl;
		*userID = localUserID;
	} else if (otherUserName == userName) {
		*userID = otherUserID;
	} else {
		return EC_USER_NOT_FOUND;
	}

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION findChannelByName_v_1_0_x(mumble_plugin_id_t callerID,
																   mumble_connection_t connection,
																   const char *channelName,
																   mumble_channelid_t *channelID) {
	calledFunctions["findChannelByName"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	if (localUserChannelName == channelName) {
		*channelID = localUserChannel;
	} else if (otherUserChannelName == channelName) {
		*channelID = otherUserChannel;
	} else {
		return EC_CHANNEL_NOT_FOUND;
	}

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getMumbleSetting_bool_v_1_0_x(mumble_plugin_id_t callerID,
																	   mumble_settings_key_t key, bool *outValue) {
	calledFunctions["getMumbleSetting_bool"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(outValue);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getMumbleSetting_int_v_1_0_x(mumble_plugin_id_t callerID,
																	  mumble_settings_key_t key, int *outValue) {
	calledFunctions["getMumbleSetting_int"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(outValue);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getMumbleSetting_double_v_1_0_x(mumble_plugin_id_t callerID,
																		 mumble_settings_key_t key, double *outValue) {
	calledFunctions["getMumbleSetting_double"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(outValue);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION getMumbleSetting_string_v_1_0_x(mumble_plugin_id_t callerID,
																		 mumble_settings_key_t key,
																		 const char **outValue) {
	calledFunctions["getMumbleSetting_string"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(outValue);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION setMumbleSetting_bool_v_1_0_x(mumble_plugin_id_t callerID,
																	   mumble_settings_key_t key, bool value) {
	calledFunctions["setMumbleSetting_bool"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(value);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION setMumbleSetting_int_v_1_0_x(mumble_plugin_id_t callerID,
																	  mumble_settings_key_t key, int value) {
	calledFunctions["setMumbleSetting_int"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(value);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION setMumbleSetting_double_v_1_0_x(mumble_plugin_id_t callerID,
																		 mumble_settings_key_t key, double value) {
	calledFunctions["setMumbleSetting_double"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(value);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION setMumbleSetting_string_v_1_0_x(mumble_plugin_id_t callerID,
																		 mumble_settings_key_t key, const char *value) {
	calledFunctions["setMumbleSetting_string"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(key);
	UNUSED(value);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION sendData_v_1_0_x(mumble_plugin_id_t callerID, mumble_connection_t connection,
														  const mumble_userid_t *users, size_t userCount,
														  const uint8_t *data, size_t dataLength, const char *dataID) {
	calledFunctions["sendData"]++;

	VERIFY_PLUGIN_ID(callerID);

	// Right now there can only be one connection managed by the current ServerHandler
	VERIFY_CONNECTION(connection);
	ENSURE_CONNECTION_SYNCHRONIZED(connection);

	for (size_t i = 0; i < userCount; i++) {
		if (users[i] != localUserID && users[i] != otherUserID) {
			return EC_USER_NOT_FOUND;
		}
	}

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION log_v_1_0_x(mumble_plugin_id_t callerID, const char *message) {
	calledFunctions["log"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(message);

	return STATUS_OK;
}

mumble_error_t PLUGIN_CALLING_CONVENTION playSample_v_1_0_x(mumble_plugin_id_t callerID, const char *samplePath) {
	calledFunctions["playSample"]++;

	VERIFY_PLUGIN_ID(callerID);

	UNUSED(samplePath);

	return EC_AUDIO_NOT_AVAILABLE;
}

MumbleAPI_v_1_0_x getMumbleAPI_v_1_0_x() {
	return { freeMemory_v_1_0_x,
			 getActiveServerConnection_v_1_0_x,
			 isConnectionSynchronized_v_1_0_x,
			 getLocalUserID_v_1_0_x,
			 getUserName_v_1_0_x,
			 getChannelName_v_1_0_x,
			 getAllUsers_v_1_0_x,
			 getAllChannels_v_1_0_x,
			 getChannelOfUser_v_1_0_x,
			 getUsersInChannel_v_1_0_x,
			 getLocalUserTransmissionMode_v_1_0_x,
			 isUserLocallyMuted_v_1_0_x,
			 isLocalUserMuted_v_1_0_x,
			 isLocalUserDeafened_v_1_0_x,
			 getUserHash_v_1_0_x,
			 getServerHash_v_1_0_x,
			 getUserComment_v_1_0_x,
			 getChannelDescription_v_1_0_x,
			 requestLocalUserTransmissionMode_v_1_0_x,
			 requestUserMove_v_1_0_x,
			 requestMicrophoneActivationOverwrite_v_1_0_x,
			 requestLocalMute_v_1_0_x,
			 requestLocalUserMute_v_1_0_x,
			 requestLocalUserDeaf_v_1_0_x,
			 requestSetLocalUserComment_v_1_0_x,
			 findUserByName_v_1_0_x,
			 findChannelByName_v_1_0_x,
			 getMumbleSetting_bool_v_1_0_x,
			 getMumbleSetting_int_v_1_0_x,
			 getMumbleSetting_double_v_1_0_x,
			 getMumbleSetting_string_v_1_0_x,
			 setMumbleSetting_bool_v_1_0_x,
			 setMumbleSetting_int_v_1_0_x,
			 setMumbleSetting_double_v_1_0_x,
			 setMumbleSetting_string_v_1_0_x,
			 sendData_v_1_0_x,
			 log_v_1_0_x,
			 playSample_v_1_0_x };
}
}; // namespace API_Mock
