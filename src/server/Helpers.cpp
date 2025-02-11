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
