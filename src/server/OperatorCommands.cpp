//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"
#include "Client.hpp"

/* --------------------------------------------------------------------------------- */
/* Operator Commands                                                                 */
/* --------------------------------------------------------------------------------- */
// kicks a user from a channel
void Server::Kick(int clientSocket, const std::vector<std::string>& tokens) {
	// Need at least channel + user.
	if (tokens.size() < 2) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 461 KICK :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	std::string channelName = tokens[0];
	std::string userName    = tokens[1];
	// Build optional reason from any remaining tokens.
	std::string reason = (tokens.size() > 2) ? tokens[2] : "Kicked";
	for (size_t i = 3; i < tokens.size(); i++) {
		reason += " " + tokens[i];
	}

	// Confirm channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 404 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	Channel &channel = _channels[channelName];

	// Confirm user is an operator in the channel.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	if (userName == _clients[clientSocket].GetNickName()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 417 :You cannot kick yourself\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Look up the user’s FD by nickname.
	int userFd = _findClientFromNickname(userName);
	if (userFd == -1 || std::find(channel.GetUsers().begin(), channel.GetUsers().end(), userFd) == channel.GetUsers().end()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 441 " + userName + " :They aren't in the channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Notify the kicked user.
	std::string kickMsg = ":" + _clients[clientSocket].GetNickName() + " KICK " + channelName + " " + userName + " :" + reason + "\r\n";
	send(userFd, kickMsg.c_str(), kickMsg.size(), 0);

	// Remove the user from the channel.
	channel.RemoveUser(userFd);

	// Broadcast to remaining channel members.
	for (int memberFd : channel.GetUsers()) {
		if (memberFd != userFd)
			send(memberFd, kickMsg.c_str(), kickMsg.size(), 0);
	}
}

// invites a user to a channel
void Server::Invite(int clientSocket, const std::vector<std::string>& tokens) {
	/*
	* Expected usage in IRC:
	*   /invite <nickname> <channel> -> Invite a user to a channel
	*/

	// placeholder for the server name
	std::string serverName = "my.irc.server";

	if (tokens.size() != 2) {
		std::string err = ":" + serverName + " 461 " +
						_clients[clientSocket].GetNickName() + " INVITE :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string targetNick = tokens[0];
	std::string channelName = tokens[1];

	// 1) Check whether channel exists
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = ":" + serverName + " 403 " +
						_clients[clientSocket].GetNickName() + " " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 2) Check whether target user exists
	int targetFd = _findClientFromNickname(targetNick);
	if (targetFd == -1) {
		std::string err = ":" + serverName + " 401 " +
						_clients[clientSocket].GetNickName() + " " + targetNick + " :No such nick\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	Channel &channel = _channels[channelName];

	// 3) Check whether client is channel operator
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = ":" + serverName + " 482 " +
						_clients[clientSocket].GetNickName() + " " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 4) Check if the target user is already in the channel
	const std::vector<int>& users = channel.GetUsers();
	if (std::find(users.begin(), users.end(), targetFd) != users.end()) {
		std::string err = ":" + serverName
						  + " 443 " + _clients[targetFd].GetNickName() + " " + channelName +
						  " :is already on channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

//	// 5) Add the target user to the invited list if not already present.
	std::vector<int>& invited = channel.GetInvited();
	if (std::find(invited.begin(), invited.end(), targetFd) == invited.end()) {
		invited.push_back(targetFd);
	}

//	6) send invite message to target user
	std::string inviteMsg = ":" + _clients[clientSocket].GetNickName() + " INVITE " + targetNick + " " + channelName + "\r\n";
	send(targetFd, inviteMsg.c_str(), inviteMsg.size(), 0);
}

// sets the topic of a channel
void Server::Topic(int clientSocket, const std::vector<std::string>& tokens) {
	/*
	* Expected usage in IRC:
	*   /topic <channel>          -> Show current topic
	*   /topic <channel> :<topic> -> Set channel’s topic
	*/

	// Set up a "server name" for numeric replies (could match your actual hostname/IP).
	std::string serverName = "127.0.0.1:6667";
	std::string channelName = tokens[0];

	// 1) Check whether channel exists
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = ":" + serverName + " 403 " +
						_clients[clientSocket].GetNickName() + " " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	Channel &channel = _channels[channelName];

	// 2) Check if user is in the channel
	if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) == channel.GetUsers().end()) {
		std::string err = ":" + serverName + " 442 " +
						_clients[clientSocket].GetNickName() + " " + channelName + " :You're not on that channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 3) If no extra topic text, show the current topic
	if (tokens.size() == 1) {
		if (channel.GetTopic().empty()) {
			// 331: RPL_NOTOPIC
			std::string rplNoTopic = ":" + serverName + " 331 " +
									_clients[clientSocket].GetNickName() + " " + channelName +
									" :No topic is set\r\n";
			send(clientSocket, rplNoTopic.c_str(), rplNoTopic.size(), 0);
		} else {
			// 332: RPL_TOPIC
			std::string rplTopic = ":" + serverName + " 332 " +
								_clients[clientSocket].GetNickName() + " " + channelName +
								" :" + channel.GetTopic() + "\r\n";
			send(clientSocket, rplTopic.c_str(), rplTopic.size(), 0);

			// 333: RPL_TOPICWHOTIME (Optional but many clients expect it)
			// Format: "<nick> <channel> <whoSetTopic> <when>"
			std::string rplTopicWhoTime = ":" + serverName + " 333 " +
										_clients[clientSocket].GetNickName() + " " + channelName + " " +
										channel.GetTopicSetBy() + " " +
										std::to_string(channel.GetTopicSetTime()) + "\r\n";
			send(clientSocket, rplTopicWhoTime.c_str(), rplTopicWhoTime.size(), 0);
		}
		return;
	}

	// 4) If there is a topic text to set, require operator status (or check +t mode if you have it)
	if (channel.GetTopicOnlySettableByOperator() &&
    std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = ":" + serverName + " 482 " +
						_clients[clientSocket].GetNickName() + " " + channelName +
						" :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 5) Gather the topic text
	std::string newTopic;
	std::string sanitized;
	for (size_t i = 1; i < tokens.size(); i++) {
		std::string token = tokens[i];
		// If this is the first token, remove a leading ':' if present.
		if (i == 1 && !token.empty() && token[0] == ':') {
			token.erase(0, 1);
		}
		// Remove all control characters (ASCII below 32 and DEL)
		token.erase(std::remove_if(token.begin(), token.end(),
					 [](unsigned char c) { return c < 32 || c == 127; }),
					 token.end());
		// Add a space if needed.
		if (!sanitized.empty()) {
			// Ensure adding a space doesn't exceed 390 characters.
			if (sanitized.size() < 390) sanitized.push_back(' ');
		}
		// Append token or a substring of it if it would exceed 390 characters.
		if (sanitized.size() + token.size() > 390) {
			size_t available = 390 - sanitized.size();
			sanitized.append(token, 0, available);
			break;
		} else {
			sanitized.append(token);
		}
	}
	newTopic = sanitized;
	if (!newTopic.empty() && newTopic[0] == ':') {
		newTopic.erase(0, 1);
	}

	// 6) Record who set the topic and when (if you store it in Channel)
	channel.SetTopic(newTopic);
	channel.SetTopicSetBy(_clients[clientSocket].GetNickName());
	channel.SetTopicSetTime(std::time(nullptr));

	// 7) Broadcast the new topic to everyone in the channel
	std::string prefix = _clients[clientSocket].GetNickName() + "!" +
						_clients[clientSocket].GetUserName() + "@127.0.0.1";  // your server's userhost
	std::string topicBroadcast = ":" + prefix + " TOPIC " + channelName + " :" + newTopic + "\r\n";

	for (int userFd : channel.GetUsers()) {
		send(userFd, topicBroadcast.c_str(), topicBroadcast.size(), 0);
	}
}

