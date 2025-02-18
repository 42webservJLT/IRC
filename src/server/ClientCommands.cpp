//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"
#include "Client.hpp"

/* --------------------------------------------------------------------------------- */
/* Client Commands                                                                   */
/* --------------------------------------------------------------------------------- */

// sets the nickname of a client
void Server::Nick(int clientSocket, const std::vector<std::string>& tokens) {
	// Expect exactly one parameter for /nick
	if (tokens.size() != 1) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 461 NICK :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string oldNick = _clients[clientSocket].GetNickName();
	std::string newNick = tokens[0];

	// Check for nickname conflicts
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->first != clientSocket && it->second.GetNickName() == newNick) {
			std::string err = ":" + oldNick + " 433 " + newNick + " :Nickname is already in use\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}
	}

	// If the user tries to set the same nickname, optionally reject it
	if (newNick == oldNick) {
		std::string err = ":" + oldNick + " 433 " + newNick + " :Nickname is old Nickname\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Assign the new nickname
	_clients[clientSocket].SetNickName(newNick);

	// Broadcast the nick change to the channels
	std::string nickMsg = ":" + oldNick + " NICK :" + newNick + "\r\n";
	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		Channel &channel = it->second;
		// Only inform channels the client is already in
		_BroadcastToChannel(channel.GetName(), nickMsg);
	}

	// Attempt to register if PASS, NICK, and USER are set
	RegisterClientIfReady(clientSocket);
}

// sets the username of a client
void Server::User(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() < 4) {
		std::string err = ":" + _clients[clientSocket].GetNickName() +
						" 461 USER :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 1) Validate username (lowercase, strip special chars)
	std::string validatedUserName = tokens[0];
	std::transform(validatedUserName.begin(), validatedUserName.end(), validatedUserName.begin(), ::tolower);
	validatedUserName.erase(std::remove_if(validatedUserName.begin(),
										validatedUserName.end(),
										[](unsigned char c){ return !std::isalnum(c) && c != '_'; }),
							validatedUserName.end());

	// 2) Default hostname/server to "*"
	std::string host = (tokens.size() > 1 && !tokens[1].empty()) ? tokens[1] : "*";
	std::string server = (tokens.size() > 2 && !tokens[2].empty()) ? tokens[2] : "*";

	// 3) Handle realname (combine remaining tokens)
	std::string realName;
	for (size_t i = 3; i < tokens.size(); ++i) {
		if (!realName.empty()) realName += " ";
		realName += tokens[i];
	}

	// Remove duplicate username check; allow duplicate usernames
	// (RFCs permit duplicate usernames as long as nicknames differ)
	_clients[clientSocket].SetUserName(validatedUserName);

	// Check if PASS was correct & nick is set, then send welcome.
	RegisterClientIfReady(clientSocket);
}

void Server::Join(int clientSocket, const std::vector<std::string>& tokens) {
	std::string serverName = "127.0.0.1:6667"; // or your real server hostname/IP

	if (!_clients[clientSocket].GetAuthenticated()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 464 JOIN :You're not authenticated\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	if (_clients[clientSocket].GetNickName().empty() || _clients[clientSocket].GetUserName().empty()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 451 JOIN :You have not registered\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	if (tokens.size() < 1 || tokens.size() > 2) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 461 JOIN :Incorrect amount for parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Utility lambda to split string by a delimiter.
	auto splitString = [](const std::string &s, char delimiter) -> std::vector<std::string> {
		std::vector<std::string> parts;
		std::istringstream iss(s);
		std::string token;
		while (std::getline(iss, token, delimiter)) {
			parts.push_back(token);
		}
		return parts;
	};

	// Split the channels (and keys, if provided) by comma.
	std::vector<std::string> channelNames = splitString(tokens[0], ',');
	std::vector<std::string> keys;
	if (tokens.size() > 1) {
		keys = splitString(tokens[1], ',');
	}

	// Process each channel join request.
	for (size_t i = 0; i < channelNames.size(); i++) {
		std::string channelName = channelNames[i];
		std::string providedKey = (i < keys.size()) ? keys[i] : "";

		// If channel name doesn't start with '#', treat as private message channel.
		if (!channelName.empty() && !channelPrefixCheck(channelName)) {
			std::string senderNick = _clients[clientSocket].GetNickName();
			std::string targetNick = channelName;
			std::vector<std::string> nicks = {senderNick, targetNick};
			std::sort(nicks.begin(), nicks.end());
			std::string pmChannel = "#pm-" + nicks[0] + "-" + nicks[1];
			if (_channels.find(pmChannel) == _channels.end()) {
				std::string err = ":" + senderNick + " 404 " + targetNick + " :No private message channel with that user\r\n";
				send(clientSocket, err.c_str(), err.size(), 0);
				return;
			}
			channelName = pmChannel; // update to the PM channel name
		}

		if (channelNameCheck(channelName)) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 403 " + channelName + " :No such channel\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}

		// If channel doesn't exist, create it, set operator, etc.
		if (_channels.find(channelName) == _channels.end()) {
			_channels[channelName] = Channel();
			_channels[channelName].SetName(channelName);
			if (!providedKey.empty()) {
				_channels[channelName].SetPassword(providedKey);
			}
			_changeOperatorPrivileges(channelName, _clients[clientSocket].GetNickName(), true);
		}

		Channel& channel = _channels[channelName];

		// Check if user is already on that channel
		if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) != channel.GetUsers().end()) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 443 " + channelName + " :You are already on that channel\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			continue;
		}

		// Check +l (user limit)
		if (channel.GetUserLimit() > NO_USER_LIMIT && channel.GetUsers().size() >= channel.GetUserLimit()) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 471 " + channelName
				+ " :Cannot join channel, user limit exceeded (+l)\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			continue;
		}

		// Check +i (invite-only)
		if (channel.GetInviteOnly()) {
			const std::vector<int>& invitedList = channel.GetInvited();
			if (std::find(invitedList.begin(), invitedList.end(), clientSocket) == invitedList.end()) {
				std::string err = ":" + _clients[clientSocket].GetNickName() + " 473 " + channelName
					+ " :Cannot join channel, invite is required (+i)\r\n";
				send(clientSocket, err.c_str(), err.size(), 0);
				continue;
			}
		}

		// Check +k (channel password)
		if (!channel.GetPassword().empty() && providedKey != channel.GetPassword()) {
			std::string err = _errMsg(_clients[clientSocket].GetNickName(), "475", channelName, "Cannot join channel (+k)");
			send(clientSocket, err.c_str(), err.size(), 0);
			continue;
		}

		// Add user to channel and remove them from Invited list
		channel.AddUser(clientSocket);
		std::vector<int>& invited = channel.GetInvited();
		invited.erase(std::remove(invited.begin(), invited.end(), clientSocket), invited.end());

		// Broadcast join
		std::string joinMsg = ":" + _clients[clientSocket].GetNickName() + " JOIN :" + channelName + "\r\n";
		_BroadcastToChannel(channelName, joinMsg);

		// NEW: Send RPL_TOPIC info after user joins (WeeChat & most IRC clients expect this).
		if (channel.GetTopic().empty()) {
			// 331: RPL_NOTOPIC
			std::string rplNoTopic = ":" + serverName + " 331 " +
				_clients[clientSocket].GetNickName() + " " + channelName + " :No topic is set\r\n";
			send(clientSocket, rplNoTopic.c_str(), rplNoTopic.size(), 0);
		} else {
			// 332: RPL_TOPIC
			std::string rplTopic = ":" + serverName + " 332 " +
				_clients[clientSocket].GetNickName() + " " + channelName + " :" + channel.GetTopic() + "\r\n";
			send(clientSocket, rplTopic.c_str(), rplTopic.size(), 0);

			// 333: RPL_TOPICWHOTIME (optional but expected by many IRC clients)
			// Format: 333 <nick> <channel> <whoSetTopic> <when>
			std::string rplTopicWhoTime = ":" + serverName + " 333 " +
				_clients[clientSocket].GetNickName() + " " + channelName + " " +
				channel.GetTopicSetBy() + " " + std::to_string(channel.GetTopicSetTime()) + "\r\n";
			send(clientSocket, rplTopicWhoTime.c_str(), rplTopicWhoTime.size(), 0);
		}
	}
}


// sends a private message
void Server::PrivMsg(int clientSocket, const std::vector<std::string>& tokens) {
	// 1) Ensure user is authenticated.
	if (!_clients[clientSocket].GetAuthenticated()) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 464 PRIVMSG :You are not authenticated\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 2) Check for correct parameter count.
	if (tokens.size() < 2) {
		std::string err = ":" + _clients[clientSocket].GetNickName() + " 461 PRIVMSG :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// 3) Parse target and message.
	std::string target = tokens[0];
	std::string message = tokens[1];
	for (size_t i = 2; i < tokens.size(); ++i) {
		message += " " + tokens[i];
	}

	// If the user typed a channel name without '#', add '#' if that channel exists
	if (!target.empty() && target[0] != '#') {
		std::string candidate = "#" + target;
		if (_channels.find(target) != _channels.end()) {
			target = candidate;
		}
	}

	// Build a more standard prefix like: nick!user@host
	std::string prefix = _clients[clientSocket].GetNickName() + "!" +
						 _clients[clientSocket].GetUserName() +
						 "@127.0.0.1"; // or your actual IP/host

	// 4) Full message to relay: ":<nick!user@host> PRIVMSG <target> :<message>"
	std::string fullMsg = ":" + prefix + " PRIVMSG " + target + " :" + message + "\r\n";

	// 5) If target is a channel, ensure it exists and user is in it, then broadcast.
	if (!target.empty() && target[0] == '#') {
		if (_channels.find(target) == _channels.end()) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 403 " + target + " :No such channel\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}
		Channel &channel = _channels[target];
		if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) == channel.GetUsers().end()) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 442 " + target + " :You're not on that channel\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}
		// Broadcast to all members in the channel (including sender).
		for (int userFd : channel.GetUsers()) {
			if (userFd == clientSocket)
				continue; // Skip sending to sender to avoid duplicate display.
			if (send(userFd, fullMsg.c_str(), fullMsg.size(), 0) == -1) {
				perror("Error sending PRIVMSG to channel user");
				// Remove user from channel and log
				std::cout << "Removing user " << userFd << " from channel " << channel.GetName()
						  << " due to send failure" << std::endl;
				channel.RemoveUser(userFd);
				// Consider also removing from other channels and closing connection
				HandleDisconnection(userFd);
			}
		}
	} else {
		// 6) Otherwise, treat as direct message to a nick.
		int targetFd = _findClientFromNickname(target);
		if (targetFd == -1) {
			std::string err = ":" + _clients[clientSocket].GetNickName() + " 401 " + target + " :No such nick\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}
		if (send(targetFd, fullMsg.c_str(), fullMsg.size(), 0) == -1) {
			perror("Error sending PRIVMSG to user");
			// Remove user and close connection
			// std::cout << "Removing user " << targetFd << " due to send failure" << std::endl;
			HandleDisconnection(targetFd);
		}
	}
}

void Server::_BroadcastToChannel(const std::string &channelName, const std::string &msg) {
	if (_channels.find(channelName) != _channels.end()) {
		Channel &channel = _channels[channelName];
		for (int userFd : channel.GetUsers()) {
			send(userFd, msg.c_str(), msg.size(), 0);
			// std::cout << "Broadcasting to " << userFd << ": " << msg;
		}
	}
}


void Server::Quit(int clientSocket, const std::vector<std::string>& /*tokens*/) {
	std::string quitMessage = "Client Quit";
	std::string broadcastMsg = ":" + _clients[clientSocket].GetNickName() +
							   " QUIT :" + quitMessage + "\r\n";

	for (auto &it : _channels) {
		Channel &channel = it.second;
		// Check if user is in the channel
		if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) != channel.GetUsers().end()) {
			// Broadcast the QUIT message to everyone in that channel
			_BroadcastToChannel(channel.GetName(), broadcastMsg);
			// Remove the user from the channel
			channel.RemoveUser(clientSocket);
		}
	}

	// Log and disconnect the client
	// std::cout << "Client " << _clients[clientSocket].GetNickName() << " (" << clientSocket << ") quitting: " << quitMessage << std::endl;
	HandleDisconnection(clientSocket);
}
