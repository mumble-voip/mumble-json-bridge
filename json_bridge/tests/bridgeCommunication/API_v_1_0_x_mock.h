#ifndef API_MOCK_H_
#define API_MOCK_H_

#include <mumble/plugin/internal/MumbleAPI_v_1_0_x.h>

#include <string>
#include <unordered_map>

namespace API_Mock {
extern std::unordered_map< std::string, int > calledFunctions;

static constexpr mumble_plugin_id_t pluginID         = 42;
static constexpr mumble_connection_t activeConnetion = 13;
static constexpr mumble_userid_t localUserID         = 5;
static constexpr mumble_userid_t otherUserID         = 7;
static constexpr mumble_channelid_t localUserChannel = 244;
static constexpr mumble_channelid_t otherUserChannel = 243;
static const std::string localUserName               = "Local user";
static const std::string otherUserName               = "Other user";
static const std::string localUserChannelName        = "Channel of local user";
static const std::string otherUserChannelName        = "Channel of other user";
static const std::string localUserChannelDesc        = "Channel of local user (description)";
static const std::string otherUserChannelDesc        = "Channel of other user (description)";

MumbleAPI_v_1_0_x getMumbleAPI_v_1_0_x();

}; // namespace API_Mock

#endif // API_MOCK_H_
