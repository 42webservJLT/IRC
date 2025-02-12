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
	std::cout << "kick command" << "\n";
	// Need at least channel + user.
	if (tokens.size() < 2) {
		std::string err = "461 KICK :Not enough parameters\r\n";
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
		std::string err = "404 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	Channel &channel = _channels[channelName];

	// Confirm user is an operator in the channel.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = "482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::cout << "is operator: " << "\n";

	// Look up the user’s FD by nickname.
	int userFd = _findClientFromNickname(userName);
	if (userFd == -1 || std::find(channel.GetUsers().begin(), channel.GetUsers().end(), userFd) == channel.GetUsers().end()) {
		std::string err = "441 " + userName + " :They aren't in the channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::cout << "user found: " << "\n";

	// Notify the kicked user.
	std::string kickMsg = ":" + _clients[clientSocket].GetNickName() + " KICK " + channelName + " " + userName + " :" + reason + "\r\n";
	std::cout << "kick message: " << kickMsg << "\n";
	send(userFd, kickMsg.c_str(), kickMsg.size(), 0);

	std::cout << "sent kick message to kicked user: " << "\n";

	// Remove the user from the channel.
	channel.RemoveUser(userFd);

	std::cout << "removed user from channel: " << "\n";

	// Broadcast to remaining channel members.
	for (int memberFd : channel.GetUsers()) {
		send(memberFd, kickMsg.c_str(), kickMsg.size(), 0);
	}

	std::cout << "broadcasted kick message to remaining channel members: " << "\n";
}

// invites a user to a channel
void Server::Invite(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() != 2) {
		std::string err = "IRC 461 INVITE :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string channelName = tokens[0];
	std::string targetNick = tokens[1];

	// 1) Check whether channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = "IRC 403 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 2) Check whether target user exists.
	int targetFd = _findClientFromNickname(targetNick);
	if (targetFd == -1) {
		std::string err = "IRC 401 " + targetNick + " :No such nick\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	Channel &channel = _channels[channelName];

	// 3) Check whether client is channel operator.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = "IRC 482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 4) Check if the target user is already in the channel.
	const std::vector<int>& users = channel.GetUsers();
	if (std::find(users.begin(), users.end(), targetFd) != users.end()) {
		// Mimics weechat's handling when user is already present.
		std::string err = "IRC 443 " + _clients[targetFd].GetNickName() + " " + channelName + " :is already on channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 5) Add the target to the invited list if not already present.
	std::vector<int>& invited = channel.GetInvited();
	if (std::find(invited.begin(), invited.end(), targetFd) == invited.end()) {
		invited.push_back(targetFd);
	}

	// 6) Send an INVITE command to the invited user, so it appears in their buffer.
	std::string inviteMsg = ":" + _clients[clientSocket].GetNickName() +
							" INVITE " + targetNick + " " + channelName + "\r\n";
	send(targetFd, inviteMsg.c_str(), inviteMsg.size(), 0);

	// 7) Optionally also send a NOTICE to the invited user.
	std::string noticeMsg = "NOTICE " + targetNick + " :You have been invited to " + channelName +
							" by " + _clients[clientSocket].GetNickName() + "\r\n";
	send(targetFd, noticeMsg.c_str(), noticeMsg.size(), 0);

	// 8) Send numeric 341 confirmation to the inviter.
	std::string confirmation = "IRC 341 " + _clients[clientSocket].GetNickName() +
							   " " + targetNick + " " + channelName + "\r\n";
	send(clientSocket, confirmation.c_str(), confirmation.size(), 0);
}

// sets the topic of a channel
void Server::Topic(int clientSocket, const std::vector<std::string>& tokens) {
	/*
	* Expected usage:
	*   /topic <channel>       -> Show current topic
	*   /topic <channel> :text -> Set channel’s topic
	*/

	// We need at least the channel name.
	if (tokens.size() < 1) {
		std::string err = "461 TOPIC :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string channelName = tokens[0];

	// Check whether channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = "403 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	Channel &channel = _channels[channelName];

	// Check if user is in the channel.
	if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) == channel.GetUsers().end()) {
		std::string err = "442 " + channelName + " :You're not on that channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// If no topic text is provided, show current topic.
	// tokens.size() == 1 means just "/topic <channel>" with no text.
	if (tokens.size() == 1) {
		if (channel.GetTopic().empty()) {
			// 331: RPL_NOTOPIC
			// "<channel> :No topic is set"
			std::string rpl = "331 " + _clients[clientSocket].GetNickName() + " " + channelName + " :No topic is set\r\n";
			send(clientSocket, rpl.c_str(), rpl.size(), 0);
		} else {
			// 332: RPL_TOPIC
			// "<channel> :<topic>"
			std::string rpl = "332 " + _clients[clientSocket].GetNickName() + " " + channelName + " :" + channel.GetTopic() + "\r\n";
			send(clientSocket, rpl.c_str(), rpl.size(), 0);
		}
		return;
	}

	// If there is a topic to set, check operator privileges if your server enforces +t.
	// For simplicity, we'll always require operator status:
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket)
		== channel.GetOperators().end()) {
		std::string err = "482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Gather the topic text (everything after the channel).
	// Typically in IRC, it's "/topic #channel :New Topic" => tokens[1] starts with a colon.
	std::string newTopic;
	{ 
		// Reconstruct from tokens[1..end].
		// A typical split might put ":New" in tokens[1], "Topic" in tokens[2], etc.
		// So we just combine them with spaces, trimming the leading colon if present.
		for (size_t i = 1; i < tokens.size(); i++) {
			if (i > 1) newTopic += " ";
			newTopic += tokens[i];
		}
		if (!newTopic.empty() && newTopic[0] == ':')
			newTopic.erase(0, 1);
	}

	// Set the topic on the channel.
	channel.SetTopic(newTopic);

	// Construct the standard IRC topic broadcast:
	// :nick!user@host TOPIC <channel> :<newtopic>
	std::string prefix = _clients[clientSocket].GetNickName() + "!" +
						_clients[clientSocket].GetUserName() + "@127.0.0.1"; 
	std::string topicMsg = ":" + prefix + " TOPIC " + channelName + " :" + newTopic + "\r\n";

	// Broadcast to all users in the channel.
	for (int userFd : channel.GetUsers()) {
		send(userFd, topicMsg.c_str(), topicMsg.size(), 0);
	}
}
