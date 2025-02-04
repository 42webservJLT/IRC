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
	_methods.insert({AUTHENTICATE, &Server::Authenticate});
	_methods.insert({NICK, &Server::Nick});
	_methods.insert({USER, &Server::User});
	_methods.insert({JOIN, &Server::Join});
	_methods.insert({PRIVMSG, &Server::PrivMsg});
	_methods.insert({KICK, &Server::Kick});
	_methods.insert({INVITE, &Server::Invite});
	_methods.insert({TOPIC, &Server::Topic});
	_methods.insert({MODE, &Server::Mode});
	_methods.insert({PING, &Server::Ping});
	_methods.insert({INVALID, nullptr});

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

/* --------------------------------------------------------------------------------- */
/* Connection Handling                                                               */
/* --------------------------------------------------------------------------------- */
// handles a new connection
void Server::HandleNewConnection() {
	sockaddr_in clientAddr;
	socklen_t addrSize = sizeof(clientAddr);
	int clientFd = accept(_listeningFd, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize);
	if (clientFd >= 0) {
		fcntl(clientFd, F_SETFL, O_NONBLOCK);
		// Register new client in poll
		struct pollfd pfd;
		pfd.fd = clientFd;
		pfd.events = POLLIN;
		_pollFds.push_back(pfd);

		_clients[clientFd] = Client(clientFd);
		std::cout << "New connection from FD: " << clientFd << std::endl;
	}
	else if (errno != EAGAIN && errno != EWOULDBLOCK) {
		std::cerr << "Failed to accept new connection: " << strerror(errno) << std::endl;
		// Don’t exit here—just log the error
	}
}

// handles a connection
void Server::HandleConnection(int clientSocket) {
	char buffer[MAX_BUFFER_SIZE + 1];
	// std::memset(buffer, 0, MAX_BUFFER_SIZE + 1);
	std::memset(buffer, 0, sizeof(buffer));
	ssize_t bytesRead = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
	if (bytesRead > 0) {
		std::string msg(buffer, bytesRead);

		std::string& clientBuffer = _clients[clientSocket].GetMsgBuffer();
		clientBuffer.append(msg);

//		for real irc client, check for \r\n instead!
		size_t pos;
		while ((pos = clientBuffer.find("\r\n")) != std::string::npos) {
			std::string commandLine = clientBuffer.substr(0, pos);
			clientBuffer.erase(0, pos + 2); // Remove processed command

			// Parse commandLine into tokens
			std::tuple<Method, std::vector<std::string>> vals = _parser.parse(commandLine);

			// Handle message
			if (std::get<0>(vals) == INVALID) {
				std::string err = "421 " + _clients[clientSocket].GetNickName() + " " + std::get<1>(vals).front() + " :Unknown command\r\n";
				send(clientSocket, err.c_str(), err.size(), 0);
				continue; // Continue processing other commands
			}

			// Execute the corresponding command handler
			(this->*_methods[std::get<0>(vals)])(clientSocket, std::get<1>(vals));
		}
	} else if (bytesRead == 0) {
		HandleDisconnection(clientSocket);
	} else {
		throw std::runtime_error("Failed to receive message");
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

void Server::RemoveClient(int clientFd) {
	close(clientFd);
	_clients.erase(clientFd);

	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it) {
		if (it->fd == clientFd) {
			_pollFds.erase(it);
			break;
		}
	}
	std::cout << "Client disconnected on FD: " << clientFd << std::endl;
}

bool Server::HandleClient(int clientFd) {
	try {
		// Directly call the existing parse-and-respond logic:
		HandleConnection(clientFd);
	} catch (const std::exception &e) {
		std::cerr << "Error handling client: " << e.what() << std::endl;
		return false;
	}
	return true;
}

// handle PING
void Server::Ping(int clientFd, const std::vector<std::string> tokens) {
	// If no parameter was sent, ignore or send an error.
	if (tokens.size() < 1) {
		std::string err("409 :No origin specified\r\n");
		send(clientFd, err.c_str(), err.size(), 0);
		return;
	}
	// typical format: PING <server-name>
	// respond with PONG <server-name>
	std::string response = "PONG " + tokens[0] + "\r\n";
	send(clientFd, response.c_str(), response.size(), 0);
}

/* --------------------------------------------------------------------------------- */
/* Client Commands                                                                   */
/* --------------------------------------------------------------------------------- */
// authenticates a client
void Server::Authenticate(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 1 || tokens[0] != GetPassword()) {
		std::string err = "464 " + _clients[clientSocket].GetNickName() + " PASS :Password incorrect\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		HandleDisconnection(clientSocket); // Disconnect the client on failed authentication
	} else {
		_clients[clientSocket].SetAuthenticated(true);
		// Optionally, send a welcome message or further instructions
		std::string welcome = "001 " + _clients[clientSocket].GetNickName() + " :Welcome to the IRC Server\r\n";
		send(clientSocket, welcome.c_str(), welcome.size(), 0);
	}
}

// sets the nickname of a client
void Server::Nick(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 1) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
	} else {
		_clients[clientSocket].SetNickName(tokens[0]);
	}
}

// sets the username of a client
void Server::User(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() < 4) { // IRC USER command has more parameters
		std::string err = "461 USER :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
	} else {
		_clients[clientSocket].SetUserName(tokens[0]);
		// You can handle mode and real name if needed
	}
}

// joins a channel
void Server::Join(int clientSocket, const std::vector<std::string> tokens) {
	if (!_clients[clientSocket].GetAuthenticated()) {
		std::string err = "464 " + _clients[clientSocket].GetNickName() + " JOIN :You're not authenticated\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	if (_clients[clientSocket].GetNickName().empty() || _clients[clientSocket].GetUserName().empty()) {
		std::string err = "451 JOIN :You have not registered\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	if (tokens.size() != 1) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[0]) != _channels.end()) {
		auto channel = _channels[tokens[0]];
//		check whether user limit is reached; 0 means no limit
		if ((channel.GetUserLimit() > NO_USER_LIMIT && channel.GetUsers().size() <
			channel.GetUserLimit()) || channel.GetUserLimit() == NO_USER_LIMIT) {
//			check whether channel is invite only or if client is invited
			if (!channel.GetInviteOnly()) {
				channel.AddUser(clientSocket);
			} else if (std::find(channel.GetInvited().begin(), channel.GetInvited().end(), clientSocket) != channel.GetInvited().end()) {
				channel.AddUser(clientSocket);
				auto invited = channel.GetInvited();
				invited.erase(std::remove(invited.begin(), invited.end(), clientSocket), invited.end());
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
	if (tokens.size() < 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, strlen(ERR_MSG_INVALID_COMMAND), 0);
		return;
	}

	std::string target = tokens[0];
	std::string message = tokens[1];
	// Reconstruct message if there are more tokens
	for (size_t i = 2; i < tokens.size(); ++i) {
		message += " " + tokens[i];
	}

	// Check if target is a channel
	if (target[0] == '#') {
		if (_channels.find(target) == _channels.end()) {
			send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, strlen(ERR_MSG_CHANNEL_NOT_FOUND), 0);
			return;
		}

		Channel &channel = _channels[target];
		if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) == channel.GetUsers().end()) {
			send(clientSocket, ERR_MSG_USER_NOT_IN_CHANNEL, strlen(ERR_MSG_USER_NOT_IN_CHANNEL), 0);
			return;
		}

		// Forward message to all users in the channel except the sender
		for (int userFd : channel.GetUsers()) {
			if (userFd != clientSocket) {
				std::string fullMessage = "PRIVMSG " + target + " :" + message + "\n";
				send(userFd, fullMessage.c_str(), fullMessage.size(), 0);
			}
		}
	}
	// Target is a user
	else {
		int targetFd = _findClientFromNickname(target);
		if (targetFd == -1) {
			send(clientSocket, ERR_MSG_USER_NOT_FOUND, strlen(ERR_MSG_USER_NOT_FOUND), 0);
			return;
		}

		// Send the message to the target user
		std::string fullMessage = "PRIVMSG " + target + " :" + message + "\n";
		send(targetFd, fullMessage.c_str(), fullMessage.size(), 0);
	}
}

/* --------------------------------------------------------------------------------- */
/* Operator Commands                                                                 */
/* --------------------------------------------------------------------------------- */
// kicks a user from a channel
void Server::Kick(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[0]) != _channels.end()) {
		int userFd = _findClientFromNickname(tokens[1]);
		if (userFd == -1) {
			send(clientSocket, ERR_MSG_USER_NOT_IN_CHANNEL, std::string(ERR_MSG_USER_NOT_IN_CHANNEL).size(), 0);
			return;
		}
		auto channel = _channels[tokens[0]];
//		check whether user is operator
		if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) != channel
		.GetOperators()
		.end()) {
//			check whether user is in channel
			if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), userFd) != channel.GetUsers().end()) {
				channel.RemoveUser(userFd);
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
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[0]) != _channels.end()) {
		int userFd = _findClientFromNickname(tokens[1]);
		if (userFd == -1) {
			send(clientSocket, ERR_MSG_USER_NOT_FOUND, std::string(ERR_MSG_USER_NOT_FOUND).size(), 0);
			return;
		}
		auto channel = _channels[tokens[0]];
//		check whether user is operator
		if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) != channel
		.GetOperators().end()) {
			std::vector<int>& invited = channel.GetInvited();
			invited.push_back(userFd);
			std::string msg = INVITED_MSG(tokens[0], tokens[1]);
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
	if (tokens.size() != 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[0]) != _channels.end()) {
		auto channel = _channels[tokens[0]];
//		check whether user is operator
		if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) != channel.GetOperators().end()) {
			channel.SetTopic(tokens[1]);
		} else {
			send(clientSocket, ERR_MSG_NOT_A_CHANNEL_OPERATOR, std::string(ERR_MSG_NOT_A_CHANNEL_OPERATOR).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

// sets the mode of a channel
void Server::Mode(int clientSocket, const std::vector<std::string> tokens) {
	if (tokens.size() < 1) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
//	check whether channel exists
	if (_channels.find(tokens[0]) != _channels.end()) {
		auto channel = _channels[tokens[0]];
//		check whether user is operator
		if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) != channel.GetOperators().end()) {
			try {
				enum Mode mode = _strToModeEnum(tokens[0]);
				switch (mode) {
					case MAKE_INVITE_ONLY:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changeInviteOnlyRestriction(tokens[0], true);
						break;
					case UNMAKE_INVITE_ONLY:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changeInviteOnlyRestriction(tokens[0], false);
						break;
					case MAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changeTopicRestriction(tokens[0], true);
						break;
					case UNMAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changeTopicRestriction(tokens[0], false);
						break;
					case GIVE_OPERATOR_PRIVILEGES:
						if (tokens.size() != 2) {
							throw std::runtime_error("Invalid command");
						}
						_changeOperatorPrivileges(tokens[0], tokens[1], true);
						break;
					case TAKE_OPERATOR_PRIVILEGES:
						if (tokens.size() != 2) {
							throw std::runtime_error("Invalid command");
						}
						_changeOperatorPrivileges(tokens[0], tokens[1], false);
						break;
					case SET_USER_LIMIT:
						if (tokens.size() != 2) {
							throw std::runtime_error("Invalid command");
						}
						_changeUserLimitRestriction(tokens[0], std::stoul(tokens[1]));
						break;
					case UNSET_USER_LIMIT:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changeUserLimitRestriction(tokens[0], NO_USER_LIMIT);
						break;
					case SET_PASSWORD:
						if (tokens.size() != 2) {
							throw std::runtime_error("Invalid command");
						}
						_changePasswordRestriction(tokens[0], tokens[1]);
						break;
					case UNSET_PASSWORD:
						if (tokens.size() != 1) {
							throw std::runtime_error("Invalid command");
						}
						_changePasswordRestriction(tokens[0], "");
						break;
					default:
						send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
						break;
				}
			} catch (std::exception &e) {
				send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
			}
		} else {
			send(clientSocket, ERR_MSG_NOT_A_CHANNEL_OPERATOR, std::string(ERR_MSG_NOT_A_CHANNEL_OPERATOR).size(), 0);
		}
	} else {
		send(clientSocket, ERR_MSG_CHANNEL_NOT_FOUND, std::string(ERR_MSG_CHANNEL_NOT_FOUND).size(), 0);
	}
}

//	changes the invite only restriction of a channel
void Server::_changeInviteOnlyRestriction(std::string channel, bool isInviteOnly) {
	_channels[channel].SetInviteOnly(isInviteOnly);
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

// finds a user from a nickname
int Server::_findClientFromNickname(std::string nickname) {
	for (auto it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.GetNickName() == nickname) {
			return it->first;
		}
	}
	return -1;
}

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
