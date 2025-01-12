//
// Created by Leon David Zipp on 1/7/25.
//

#include "Channel.hpp"

/* --------------------------------------------------------------------------------- */
/* Constructors & Destructors                                                        */
/* --------------------------------------------------------------------------------- */

Channel::Channel() : _name(""), _password(""), _topic(""), _inviteOnly(false), _topicOnlySettableByOperator(false) {}

Channel::~Channel() {}

/* --------------------------------------------------------------------------------- */
/* Getters & Setters                                                                 */
/* --------------------------------------------------------------------------------- */
// returns the name of the channel
std::string Channel::GetName() const {
	return _name;
}

// returns the password of the channel
std::string Channel::GetPassword() const {
	return _password;
}

// returns the topic of the channel
std::string Channel::GetTopic() const {
	return _topic;
}

// returns if channel is invite only
bool Channel::GetInviteOnly() const {
	return _inviteOnly;
}

// returns the user limit of the channel
size_t Channel::GetUserLimit() const {
	return _userLimit;
}

// returns the messages of the channel
std::vector<std::string>& Channel::GetMessages() {
	return _messages;
}

// returns the operators of the channel
std::vector<Client&>& Channel::GetOperators() {
	return _operators;
}

// returns the users of the channel
std::vector<Client&>& Channel::GetUsers() {
	return _users;
}

// returns the invited users of the channel
std::vector<Client&>& Channel::GetInvited() {
	return _invited;
}

// sets the name of the channel
void Channel::SetName(std::string name) {
	_name = name;
}

// sets the password of the channel
void Channel::SetPassword(std::string password) {
	_password = password;
}

// sets the topic of the channel
void Channel::SetTopic(std::string topic) {
	_topic = topic;
}

// sets if channel is invite only
void Channel::SetInviteOnly(bool inviteOnly) {
	_inviteOnly = inviteOnly;
}

// sets the user limit of the channel
void Channel::SetUserLimit(size_t userLimit) {
	_userLimit = userLimit;
}

// adds a user to the channel
void Channel::AddUser(int user) {
	_users.push_back(user);
}

// makes a user operator of the channel
void Channel::MakeOperator(int user) {
	// TODO: check whether operator needs to have been a user first
	_operators.push_back(user);
}

// removes operator status from a user
void Channel::RemoveOperator(const int user) {
	auto it = std::find_if(_operators.begin(), _operators.end(), [user](const Client& c) {
		return c.GetUserId() == user;
	});
	if (it != _operators.end()) {
		_operators.erase(it);
	}
}

// removes a user from the channel
void Channel::RemoveUser(const int user) {
	auto it = std::find_if(_users.begin(), _users.end(), [&](const Client& c) {
		return c.GetUserName() == user;
	});
	if (it != _users.end()) {
		_users.erase(it);
	}
}

// gets the Password of the channel
std::string Channel::GetPassword() const {
	return _password;
}
