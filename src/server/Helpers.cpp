//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"

// converts a string to a mode enum
Mode _strToModeEnum(std::string str) {
	if (str == "+i") {
		return MAKE_INVITE_ONLY;
	} else if (str == "-i") {
		return UNMAKE_INVITE_ONLY;
	} else if (str == "+t") {
		return MAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR;
	} else if (str == "-t") {
		return UNMAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR;
	} else if (str == "+o") {
		return GIVE_OPERATOR_PRIVILEGES;
	} else if (str == "-o") {
		return TAKE_OPERATOR_PRIVILEGES;
	} else if (str == "+l") {
		return SET_USER_LIMIT;
	} else if (str == "-l") {
		return UNSET_USER_LIMIT;
	} else if (str == "+k") {
		return SET_PASSWORD;
	} else if (str == "-k") {
		return UNSET_PASSWORD;
	} else {
		return INVALID_MODE;
	}
}

// finds a user from a nickname
int Server::_findClientFromNickname(std::string nickname) {
	for (auto it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.GetNickName() == nickname) {
			return it->first;
		}
	}
	return -1;
}

std::string _errMsg(const std::string& nick, const std::string& code, const std::string& msg, const std::string&
reason) {
	std::ostringstream err;
	err << ":" << nick << " " << code << " " << msg << " :" << reason << "\r\n";
	return err.str();
}

bool Server::channelPrefixCheck(const std::string& channelName) {
	if (channelName) {
		return channelName[0] == '#' || channelName[0] == '&' || channelName[0] == '!' || channelName[0] == '+';
	}
	return false;
}

bool Server::channelNameCheck(const std::string& channelName) {
	if (channelName) {
		if (!channelPrefixCheck(channelName)) {
			return false;
		}
		if (channelName.find(' ') != std::string::npos) { //can't happen because of token splitting but just in case
			return false;
		}
		for (char c : channelName) {
			if (iscntrl(c)) {
				return false;
			}
		}
		if (channelName.size() > 200 || channelName.size() > 1) { // Assuming 50 is the maximum length for a channel name
			return false;
		}
		return true;
	}
	return false;
}