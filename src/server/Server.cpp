//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"

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

	//	initialize pollfd vector
	_pollFds.push_back({_socket, POLLIN, 0});

//	initialize function mapping
	_methods.insert({AUTHENTICATE, &Server::Authenticate});
	_methods.insert({NICK, &Server::Nick});
	_methods.insert({USER, &Server::User});
	_methods.insert({JOIN, &Server::Join});
	_methods.insert({PRIVMSG, &Server::PrivMsg});
	_methods.insert({KICK, &Server::Kick});
	_methods.insert({INVITE, &Server::Invite});
	_methods.insert({TOPIC, &Server::Topic});
	_methods.insert({MODE, &Server::Mode});
	_methods.insert({INVALID, nullptr});

//	initialize parser
//	_parser = Parser();
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
void Server::Run() {
//	start listening
	if (listen(_socket, MAX_CONNECTIONS) < 0) {
		close(_socket);
		throw std::runtime_error("Failed to listen");
	}

//	announce server is running
	std::cout << "Server running on " << GetHost() << ":" << GetPort() << std::endl;

//	start polling
	while (true) {
		int pollCount = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollCount < 0) {
			close(_socket);
			throw std::runtime_error("Failed to poll");
		}

//		handle incoming connections
		for (auto it = _pollFds.begin(); it != _pollFds.end(); ++it) {
			switch (it->revents) {
				case POLLIN:
					if (it->fd == _socket) {
//						new client connection
						HandleNewConnection();
					} else {
//						old client connection
						HandleConnection(it->fd);
					}
					break;
				case POLLHUP:
//					client disconnected
					HandleDisconnection(it->fd);
					break;
				case 0:
					continue;
				default:
					break;
			}

		}
	}
}

/* --------------------------------------------------------------------------------- */
/* Connection Handling                                                               */
/* --------------------------------------------------------------------------------- */
// handles a new connection
void Server::HandleNewConnection() {
//	sockaddr_in clientAddr;
//	int clientFd = accept(_socket, &clientAddr, nullptr);
	int clientFd = accept(_socket, nullptr, nullptr);
	if (clientFd >= 0) {
//		set client socket to non-blocking
		fcntl(clientFd, F_SETFL, O_NONBLOCK);
		_pollFds.push_back({ clientFd, POLLIN, 0 });

		Client client;
		_clients.insert({ clientFd, client });

		std::cout << "New connection from " << clientFd << std::endl;
	}
}

// handles a connection
void Server::HandleConnection(int clientSocket) {
	char buffer[MAX_BUFFER_SIZE + 1];
	std::memset(buffer, 0, MAX_BUFFER_SIZE + 1);
	ssize_t bytesRead = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
	if (bytesRead > 0) {
		std::string msg(buffer);

		std::string& clientBuffer = _clients[clientSocket].GetMsgBuffer();
		clientBuffer.append(msg);

//		for real irc client, check for \r\n instead!
		if (msg.find("\n") != std::string::npos) {
//			verify authenticated
			if (!_clients[clientSocket].GetAuthenticated()) {
				send(clientSocket, ERR_MSG_UNAUTHENTICATED, std::string(ERR_MSG_UNAUTHENTICATED).size(), 0);
				return;
			}
//			TODO: implement
//			parse message
//			std::tuple<Method, std::vector<std::string>&> vals = _parser.Parse(clientBuffer);
//			handle message
//			if (std::get<0>(vals) == INVALID) {
//				send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
//				return;
//			}
//			(this->*_methods[std::get<0>(vals)])(clientSocket, std::get<1>(vals));
//			clear buffer
			std::cout << clientBuffer << std::endl;
			clientBuffer.clear();
		}
	} else if (bytesRead == 0) {
		HandleDisconnection(clientSocket);
	} else {
		std::cerr << "Error reading from client socket" << std::endl;
	}
}

// handles a disconnection
void Server::HandleDisconnection(int clientSocket) {
	_clients.erase(clientSocket);
	close(clientSocket);
	_pollFds.erase(std::remove_if(_pollFds.begin(), _pollFds.end(), [clientSocket](const pollfd &fd) {
		return fd.fd == clientSocket;
	}), _pollFds.end());
}

/* --------------------------------------------------------------------------------- */
/* Client Commands                                                                   */
/* --------------------------------------------------------------------------------- */
// authenticates a client
void Server::Authenticate(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 2 || tokens[1] != GetPassword()) {
		send(clientSocket, ERR_MSG_UNAUTHENTICATED, std::string(ERR_MSG_UNAUTHENTICATED).size(), 0);
	} else {
		_clients[clientSocket].SetAuthenticated(true);
	}
}

// sets the nickname of a client
void Server::Nick(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
	} else {
		_clients[clientSocket].SetNickName(tokens[1]);
	}
}

// sets the username of a client
void Server::User(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
	} else {
		_clients[clientSocket].SetUserName(tokens[1]);
	}
}

// joins a channel
void Server::Join(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[1]) != _channels.end()) {
		auto channel = _channels[tokens[1]];
//		check whether user limit is reached; 0 means no limit
		if ((channel.GetUserLimit() > NO_USER_LIMIT && channel.GetUsers().size() <
		channel.GetUserLimit()) || channel.GetUserLimit() == NO_USER_LIMIT) {
//			check whether channel is invite only or if client is invited
			if (!channel.GetInviteOnly()) {
					channel.AddUser(_clients[clientSocket].GetNickName());
			} else if (channel.GetInvited().find(_clients[clientSocket].GetNickName()) != channel.GetInvited().end()){
				channel.AddUser(_clients[clientSocket].GetNickName());
				channel.GetInvited().erase(_clients[clientSocket].GetNickName());
			} else {
				send(clientSocket, ERR_MSG_CHANNEL_NOT_INVITED, std::string(ERR_MSG_CHANNEL_NOT_INVITED).size(), 0);
			}
		} else {
			send(clientSocket, ERR_MSG_CHANNEL_FULL, std::string(ERR_MSG_CHANNEL_FULL).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

// sends a private message
void Server::PrivMsg(int clientSocket, const std::vector<std::string> tokens) {
	(void) clientSocket;
	(void) tokens;
//	TODO: implement
}

/* --------------------------------------------------------------------------------- */
/* Operator Commands                                                                 */
/* --------------------------------------------------------------------------------- */
// kicks a user from a channel
void Server::Kick(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 3) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[1]) != _channels.end()) {
		auto channel = _channels[tokens[1]];
//		check whether user is operator
		if (channel.GetOperators().find(_clients[clientSocket].GetNickName()) != channel.GetOperators().end()) {
//			check whether user is in channel
			if (channel.GetUsers().find(tokens[2]) != channel.GetUsers().end()) {
				channel.RemoveUser(tokens[2]);
			} else {
				send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
			}
		} else {
			send(clientSocket, ERR_MSG_USER_NOT_IN_CHANNEL, std::string(ERR_MSG_USER_NOT_IN_CHANNEL).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

// invites a user to a channel
void Server::Invite(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 3) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[1]) != _channels.end()) {
		auto channel = _channels[tokens[1]];
//		check whether user is operator
		if (channel.GetOperators().find(_clients[clientSocket].GetNickName()) != channel.GetOperators().end()) {
			channel.GetInvited().insert(tokens[2]);
			std::string msg = INVITED_MSG(tokens[1], tokens[2]);
			send(clientSocket, msg.c_str(), msg.size(), 0);
		} else {
			send(clientSocket, ERR_MSG_NOT_A_CHANNEL_OPERATOR, std::string(ERR_MSG_NOT_A_CHANNEL_OPERATOR).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

// sets the topic of a channel
void Server::Topic(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 3) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[1]) != _channels.end()) {
		auto channel = _channels[tokens[1]];
//		check whether user is operator
		if (channel.GetOperators().find(_clients[clientSocket].GetNickName()) != channel.GetOperators().end()) {
			channel.SetTopic(tokens[2]);
		} else {
			send(clientSocket, ERR_MSG_NOT_A_CHANNEL_OPERATOR, std::string(ERR_MSG_NOT_A_CHANNEL_OPERATOR).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

// sets the mode of a channel
void Server::Mode(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() < 3) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[1]) != _channels.end()) {
		auto channel = _channels[tokens[1];
//		check whether user is operator
		if (channel.GetOperators().find(_clients[clientSocket].GetNickName()) != channel.GetOperators().end()) {
			for (size_t i = 2; i < tokens.size(); i++) {
				Mode mode = _strToModeEnum(tokens[i]);
				switch (mode) {
					case MAKE_INVITE_ONLY:
						_changeInviteOnlyRestriction();
						break;
					case UNMAKE_INVITE_ONLY:
						_changeInviteOnlyRestriction();
						break;
					case MAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
						_changeTopicRestriction();
						break;
					case UNMAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
						_changeTopicRestriction();
						break;
					case GIVE_OPERATOR_PRIVILEGES:
						_changeOperatorPrivileges();
						break;
					case TAKE_OPERATOR_PRIVILEGES:
						_changeOperatorPrivileges();
						break;
					case SET_USER_LIMIT:
						_changeUserLimitRestriction();
						break;
					case UNSET_USER_LIMIT:
						_changeUserLimitRestriction();
						break;
					case SET_PASSWORD:
						_changePasswordRestriction();
						break;
					case UNSET_PASSWORD:
						_changePasswordRestriction();
						break;
					default:
						send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
						break;
				}
			}
		} else {
			send(clientSocket, ERR_MSG_NOT_A_CHANNEL_OPERATOR, std::string(ERR_MSG_NOT_A_CHANNEL_OPERATOR).size(), 0);
		}

}

//	changes the invite only restriction of a channel
void Server::_changeInviteOnlyRestriction(std::string channel, bool isInviteOnly) {
	_channels[channel].SetInviteOnly(mode);
}

// changes the topic restriction of a channel
void Server::_changeTopicRestriction();
void Server::_changePasswordRestriction();
void Server::_changeOperatorPrivileges();
void Server::_changeUserLimitRestriction();

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

