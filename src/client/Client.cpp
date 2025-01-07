//
// Created by Leon David Zipp on 1/7/25.
//

#include "../../inc/Client.hpp"


Client::Client() : _userName(""), _nickName(""), _authenticated(false) {}

Client::~Client() {}

// returns the username of the client
std::string Client::GetUserName() const {
	return _userName;
}

// returns the nickname of the client
std::string Client::GetNickName() const {
	return _nickName;
}

// returns if client is authenticated
bool Client::IsAuthenticated() const {
	return _authenticated;
}

// returns the channel names of channels the client has joined
std::vector<std::string> Client::GetJoinedChannels() const {
	return _joinedChannels;
}

