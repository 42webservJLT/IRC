//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"
#include "Client.hpp"

Mode _strToModeEnum(std::string str);

/* --------------------------------------------------------------------------------- */
/* Constructors & Destructors                                                        */
/* --------------------------------------------------------------------------------- */
Server::Server(uint16_t port, std::string password) : _host("127.0.0.1"), _port(port), _password(password) {
	//	open socket
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == -1) {
		throw std::runtime_error("Failed to create socket");
	}

//	set socket options
	int flags = fcntl(_socket, F_GETFL, 0);
	fcntl(_socket, F_SETFL, flags | O_NONBLOCK);

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

//	resolve hostname to IP address
	struct addrinfo *res;
	int status = getaddrinfo(GetHost().c_str(), std::to_string(GetPort()).c_str(), &hints, &res);
	if (status != 0) {
		close(_socket);
		throw std::runtime_error("Failed to resolve hostname");
	}

//	bind socket
	if (bind(_socket, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		close(_socket);
		throw std::runtime_error("Failed to bind socket");
	}

	freeaddrinfo(res);

	// IMPORTANT: listen on your bound socket
	if (listen(_socket, SOMAXCONN) < 0) {
		close(_socket);
		throw std::runtime_error("Failed to listen on socket");
	}

	// Make _listeningFd point to the same socket
	_listeningFd = _socket;

	//	initialize pollfd vector
	_pollFds.push_back({_socket, POLLIN, 0});

	// Print server start message
	std::cout << "Server running on " << _host << ":" << _port << std::endl;

//	initialize function mapping
	_methods.emplace(AUTHENTICATE, static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Authenticate));
	_methods.emplace(NICK,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Nick));
	_methods.emplace(USER,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::User));
	_methods.emplace(JOIN,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Join));
	_methods.emplace(MSG,    static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::PrivMsg));
	_methods.emplace(KICK,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Kick));
	_methods.emplace(INVITE,     static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Invite));
	_methods.emplace(TOPIC,      static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Topic));
	_methods.emplace(MODE,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Mode));
	_methods.emplace(PING,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Ping));
	_methods.emplace(QUIT,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Quit));
	_methods.emplace(INVALID,    nullptr);

//	initialize parser
	_parser = Parser();
}

Server::~Server() {}

/* --------------------------------------------------------------------------------- */
/* Getters & Setters                                                                 */
/* --------------------------------------------------------------------------------- */
// returns the host
std::string Server::GetHost() const {
	return _host;
}

// returns the port number
uint16_t Server::GetPort() const {
	return _port;
}

// returns the password
std::string Server::GetPassword() const {
	return _password;
}

// returns the socket
int Server::GetSocket() const {
	return _socket;
}

// returns the pollfd vector
std::vector<pollfd> Server::GetPollFds() const {
	return _pollFds;
}

/* --------------------------------------------------------------------------------- */
/* Run                                                                               */
/* --------------------------------------------------------------------------------- */
// runs the server
bool Server::Run() {
	while (true) {
		int pollCount = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollCount < 0) {
			// handle error
			return false;
		}

		std::vector<int> toRemove;
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].revents & POLLIN) {
				if (_pollFds[i].fd == _listeningFd) {
					HandleNewConnection();
				} else {
					if (!HandleClient(_pollFds[i].fd)) {
						toRemove.push_back(_pollFds[i].fd);
					}
				}
			}
			// optionally handle other events like POLLOUT
		}

		// Remove closed/disconnected FDs here
		for (size_t i = 0; i < toRemove.size(); ++i) {
			RemoveClient(toRemove[i]);
		}
	}
	return true;
}

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
