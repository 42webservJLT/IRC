//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"

// sets the mode of a channel
void Server::Mode(int clientSocket, const std::vector<std::string>& tokens) {
	// Expected command format: MODE <channel> <mode> [parameters...]
	if (tokens.size() < 2) {
		std::string err = "IRC 461 MODE :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string channelName = tokens[0];
	std::string modeStr = tokens[1];

	// Check whether channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = "IRC 403 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	Channel &channel = _channels[channelName];
	// Check whether client is a channel operator.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = "IRC 482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Use the enum from the global namespace to avoid name conflict with the member function.
	::Mode mode = _strToModeEnum(modeStr);

	try {
		switch (mode) {
			case MAKE_INVITE_ONLY:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> +i");
				_changeInviteOnlyRestriction(channelName, true);
				break;
			case UNMAKE_INVITE_ONLY:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> -i");
				_changeInviteOnlyRestriction(channelName, false);
				break;
			case MAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> +t");
				_changeTopicRestriction(channelName, true);
				break;
			case UNMAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> -t");
				_changeTopicRestriction(channelName, false);
				break;
			case GIVE_OPERATOR_PRIVILEGES:
				if (tokens.size() != 3)
					throw std::runtime_error("Usage: MODE <channel> +o <nick>");
				_changeOperatorPrivileges(channelName, tokens[2], true);
				break;
			case TAKE_OPERATOR_PRIVILEGES:
				if (tokens.size() != 3)
					throw std::runtime_error("Usage: MODE <channel> -o <nick>");
				_changeOperatorPrivileges(channelName, tokens[2], false);
				break;
			case SET_USER_LIMIT:
				if (tokens.size() != 3)
					throw std::runtime_error("Usage: MODE <channel> +l <limit>");
				_changeUserLimitRestriction(channelName, std::stoul(tokens[2]));
				break;
			case UNSET_USER_LIMIT:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> -l");
				_changeUserLimitRestriction(channelName, NO_USER_LIMIT);
				break;
			case SET_PASSWORD:
				if (tokens.size() != 3)
					throw std::runtime_error("Usage: MODE <channel> +k <password>");
				_changePasswordRestriction(channelName, tokens[2]);
				break;
			case UNSET_PASSWORD:
				if (tokens.size() != 2)
					throw std::runtime_error("Usage: MODE <channel> -k");
				_changePasswordRestriction(channelName, "");
				break;
			default:
			{
				std::string err = "IRC 421 MODE " + modeStr + " :Unknown MODE command\r\n";
				send(clientSocket, err.c_str(), err.size(), 0);
				return;
			}
		}

		// After a successful mode change, inform all users in the channel.
		std::string response = ":" + _clients[clientSocket].GetNickName() + " MODE " + channelName + " " + modeStr;
		if (tokens.size() > 2) {
			for (size_t i = 2; i < tokens.size(); i++) {
				response += " " + tokens[i];
			}
		}
		response += "\r\n";
		_BroadcastToChannel(channelName, response);

	} catch (std::exception &e) {
		std::string err = "IRC 461 MODE " + channelName + " :" + e.what() + "\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
	}
}

// changes the topic restriction of a channel
void Server::_changeTopicRestriction(std::string channel, bool isTopicOnlySettableByOperator) {
	_channels[channel].SetTopicOnlySettableByOperator(isTopicOnlySettableByOperator);
}

// changes the password restriction of a channel
void Server::_changePasswordRestriction(std::string channel, std::string password) {
	_channels[channel].SetPassword(password);
}

// changes the operator privileges of a channel
void Server::_changeOperatorPrivileges(std::string channel, std::string user, bool isOperator) {
	int userFd = _findClientFromNickname(user);
	if (userFd == -1) {
		throw std::runtime_error("User not found");
	}
	if (isOperator) {
		_channels[channel].MakeOperator(userFd);
	} else {
		_channels[channel].RemoveOperator(userFd);
	}
}

// changes the user limit restriction of a channel
void Server::_changeUserLimitRestriction(std::string channel, size_t userLimit) {
	_channels[channel].SetUserLimit(userLimit);
}

// changes the invite only restriction of a channel
void Server::_changeInviteOnlyRestriction(std::string channel, bool flag) {
	_channels[channel].SetInviteOnly(flag);
}
