//
// Created by Leon David Zipp on 1/7/25.
//

#include "Channel.hpp"

/* --------------------------------------------------------------------------------- */
/* Constructors & Destructors                                                        */
/* --------------------------------------------------------------------------------- */

Channel::Channel() : _name(""), _password(""), _inviteOnly(false), _topic(""), _topicOnlySettableByOperator(false) {}

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
std::vector<int>& Channel::GetOperators() {
	return _operators;
}

// returns the users of the channel
std::vector<int>& Channel::GetUsers() {
	return _users;
}

// returns the invited users of the channel
std::vector<int>& Channel::GetInvited() {
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

// returns if the topic of the channel can only be set by operators
bool Channel::GetTopicOnlySettableByOperator() const {
	return _topicOnlySettableByOperator;
}

// sets if the topic of the channel can only be set by operators
void Channel::SetTopicOnlySettableByOperator(bool topicOnlySettableByOperator) {
	_topicOnlySettableByOperator = topicOnlySettableByOperator;
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
	if (IsUserOperator(user)) {
		return;
	}
	_operators.push_back(user);
}

// removes operator status from a user
void Channel::RemoveOperator(const int user) {
	auto it = std::find(_operators.begin(), _operators.end(), user);
	if (it != _operators.end()) {
		_operators.erase(it);
	}
}

// removes a user from the channel
void Channel::RemoveUser(const int user) {
	auto it = std::find(_users.begin(), _users.end(), user);
	if (it != _users.end()) {
		_users.erase(it);
	}
}

bool Channel::IsUserOperator(int user) {
	return std::find(_operators.begin(), _operators.end(), user) != _operators.end();
}

const std::string& Channel::GetTopicSetBy() const { 
	return _topicSetBy; 
}

void Channel::SetTopicSetBy(const std::string &nick) { 
	_topicSetBy = nick; 
}

time_t Channel::GetTopicSetTime() const { 
	return _topicSetTime; 
}

void Channel::SetTopicSetTime(time_t time) { 
	_topicSetTime = time; 
}
