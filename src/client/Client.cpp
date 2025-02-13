//
// Created by Leon David Zipp on 1/7/25.
//

#include "Client.hpp"
#include <iostream>

/* --------------------------------------------------------------------------------- */
/* Constructors & Destructors                                                        */
/* --------------------------------------------------------------------------------- */
Client::Client() : _fd(-1), _userName(""), _nickName(""), _authenticated(false), _msgBuffer("") {
	std::cout << "message buffer: " << _msgBuffer << "innited" << std::endl;
}


Client::Client(int fd) : _fd(fd), _userName(""), _nickName(""), _authenticated(false), _msgBuffer("") {
}

Client::~Client() {}

/* --------------------------------------------------------------------------------- */
/* Getters & Setters                                                                 */
/* --------------------------------------------------------------------------------- */

// returns the username of the client
std::string Client::GetUserName() const {
	return _userName;
}

// returns the nickname of the client
std::string Client::GetNickName() const {
	return _nickName;
}

// returns if client is authenticated
bool Client::GetAuthenticated() const {
	return _authenticated;
}

std::string& Client::GetMsgBuffer() {
	return _msgBuffer;
}

// sets the username of the client
void Client::SetUserName(std::string userName) {
	_userName = userName;
}

// sets the nickname of the client
void Client::SetNickName(std::string nickName) {
	_nickName = nickName;
}

// sets if client is authenticated
void Client::SetAuthenticated(bool authenticated) {
	_authenticated = authenticated;
}

int Client::GetFd() const {
	return _fd;
}