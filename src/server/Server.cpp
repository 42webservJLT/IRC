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
	_methods.emplace(PRIVMSG,    static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::PrivMsg));
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
				std::string commandName = (!std::get<1>(vals).empty()) ? std::get<1>(vals).front() : "";
				std::string err = "421 " + _clients[clientSocket].GetNickName() + " " + commandName + " :Unknown command\r\n";
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
void Server::Ping(int clientFd, const std::vector<std::string>& tokens) {
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
// authenticates a client if the password is correct
void Server::Authenticate(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() != 1 || tokens[0] != GetPassword()) {
		std::string err = "464 " + _clients[clientSocket].GetNickName() + " PASS :Password incorrect\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		// PREVIOUS APPROACH: HandleDisconnection(clientSocket); // Disconnect the client on failed authentication
		RemoveClient(clientSocket); // forcibly disconnect
		return;
	} else {
		_clients[clientSocket].SetAuthenticated(true);
		// Optionally, send a welcome message or further instructions
		RegisterClientIfReady(clientSocket);
	}
}

// sets the nickname of a client
void Server::Nick(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() != 1) {
		std::string err = "461 NICK :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	_clients[clientSocket].SetNickName(tokens[0]);
	// Check if PASS was correct & user is set
	RegisterClientIfReady(clientSocket);
}

// sets the username of a client
void Server::User(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() < 4) {
		std::string err = "461 USER :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	} else {
		_clients[clientSocket].SetUserName(tokens[0]);
		// Optionally handle mode & real name

		// Check if PASS was correct & nick is set
		RegisterClientIfReady(clientSocket);
	}
}

void Server::RegisterClientIfReady(int clientSocket) {
	Client &c = _clients[clientSocket];
	if (c.GetAuthenticated() && !c.GetNickName().empty() && !c.GetUserName().empty()) {
		std::string welcome = "001 " + c.GetNickName() + " :Welcome to the IRC Server\r\n";
		send(clientSocket, welcome.c_str(), welcome.size(), 0);
	}
}

// joins a channel
void Server::Join(int clientSocket, const std::vector<std::string>& tokens) {
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
	if (tokens.size() < 1 || tokens.size() > 2) {
		std::string err = "461 JOIN :Incorrect amount for parameters\r\n";
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
	std::string channelName = tokens[0];
	std::string providedKey = (tokens.size() > 1) ? tokens[1] : "";

	Channel* channel;
	// if channel doesn't exist, create it
	if (_channels.find(channelName) == _channels.end()) {
		_channels[channelName] = Channel();
		_channels[channelName].SetName(channelName);
		// If a channel key is provided when creating the channel, set it.
		if (!providedKey.empty()) {
			_channels[channelName].SetPassword(providedKey);
		}
		// Assign the creator as an operator using _changeOperatorPrivileges
		_changeOperatorPrivileges(channelName, _clients[clientSocket].GetNickName(), true);
		std::cout << "Channel " << channelName << " created." << std::endl;
	}
	channel = &_channels[channelName];
	// check user limit; 0 means no limit
	if (channel->GetUserLimit() > NO_USER_LIMIT && channel->GetUsers().size() >= channel->GetUserLimit()) {
		std::string err = "471 " + _clients[clientSocket].GetNickName() + " " + channelName + " :Cannot join channel, User limit exceeded (+l)\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	// if channel is invite-only, require an invitation
	if (channel->GetInviteOnly() &&
		std::find(channel->GetInvited().begin(), channel->GetInvited().end(), clientSocket) == channel->GetInvited().end()) {
		std::string err = "473 " + _clients[clientSocket].GetNickName() + " " + channelName + " :Cannot join channel, invite is required (+i)\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	// If the channel has a key (password) already, check if the provided key matches.
	if (!channel->GetPassword().empty()) {
		if (providedKey != channel->GetPassword()) {
			std::string err = "475 " + _clients[clientSocket].GetNickName() + " " + channelName + " :Cannot join channel (+k)\r\n";
			send(clientSocket, err.c_str(), err.size(), 0);
			return;
		}
	}

	// check if user is already in the channel
	if (std::find(channel->GetUsers().begin(), channel->GetUsers().end(), clientSocket) != channel->GetUsers().end()) {
		std::string err = "443 " + _clients[clientSocket].GetNickName() + " " + channelName + " :You are already on that channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	// add user & remove invitation if present
	channel->AddUser(clientSocket);
	std::vector<int>& invited = channel->GetInvited();
	invited.erase(std::remove(invited.begin(), invited.end(), clientSocket), invited.end());

	// log and broadcast join
	std::cout << _clients[clientSocket].GetNickName() << " joined channel " << channelName << std::endl;
	std::string joinMsg = ":" + _clients[clientSocket].GetNickName() + " JOIN :" + channelName + "\r\n";
	_BroadcastToChannel(channelName, joinMsg);
}

// sends a private message
void Server::PrivMsg(int clientSocket, const std::vector<std::string>& tokens) {
	if (!_clients[clientSocket].GetAuthenticated()) {
		std::string err = "464 " + _clients[clientSocket].GetNickName() + " PRIVMSG :You are not authenticated\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	if (tokens.size() < 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, strlen(ERR_MSG_INVALID_COMMAND), 0);
		return;
	}
	std::string target = tokens[0];
	std::string message = tokens[1];
	for (size_t i = 2; i < tokens.size(); ++i) {
		message += " " + tokens[i];
	}
	// If target is a channel, ensure it exists and user is in it.
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
		// Broadcast to channel
		std::string fullMsg = ":" + _clients[clientSocket].GetNickName() + " PRIVMSG " + target + " :" + message + "\r\n";
		for (int userFd : channel.GetUsers()) {
			if (userFd != clientSocket) {
				send(userFd, fullMsg.c_str(), fullMsg.size(), 0);
			}
		}
	} else {
		// Direct message
		int targetFd = _findClientFromNickname(target);
		if (targetFd == -1) {
			send(clientSocket, ERR_MSG_USER_NOT_FOUND, strlen(ERR_MSG_USER_NOT_FOUND), 0);
			return;
		}
		std::string fullMsg = ":" + _clients[clientSocket].GetNickName() + " PRIVMSG " + target + " :" + message + "\r\n";
		send(targetFd, fullMsg.c_str(), fullMsg.size(), 0);
	}
}

void Server::_BroadcastToChannel(const std::string &channelName, const std::string &msg) {
	if (_channels.find(channelName) != _channels.end()) {
		Channel &channel = _channels[channelName];
		for (int userFd : channel.GetUsers()) {
			send(userFd, msg.c_str(), msg.size(), 0);
		}
	}
}

void Server::Quit(int clientSocket, const std::vector<std::string>& tokens) {
	// Determine quit message (if provided by client, remove any leading ':' character)
	std::string quitMessage = "Client Quit";
	if (!tokens.empty()) {
		quitMessage = tokens[0];
		if (!quitMessage.empty() && quitMessage[0] == ':')
			quitMessage = quitMessage.substr(1);
	}

	// Construct the quit message using the client's nickname
	std::string broadcastMsg = ":" + _clients[clientSocket].GetNickName() +
							" QUIT :" + quitMessage + "\r\n";

	// Broadcast quit message to all channels the client is in and remove from those channels
	for (auto &pair : _channels) {
		Channel &channel = pair.second;
		if (std::find(channel.GetUsers().begin(), channel.GetUsers().end(), clientSocket) != channel.GetUsers().end()) {
			_BroadcastToChannel(channel.GetName(), broadcastMsg);
			channel.RemoveUser(clientSocket);
		}
	}

	// Log the quit event on the server terminal
	std::cout << "Client " << _clients[clientSocket].GetNickName() << " (" << clientSocket << ") is quitting: " << quitMessage << std::endl;

	// Disconnect the client
	HandleDisconnection(clientSocket);
}


/* --------------------------------------------------------------------------------- */
/* Operator Commands                                                                 */
/* --------------------------------------------------------------------------------- */
// kicks a user from a channel
void Server::Kick(int clientSocket, const std::vector<std::string>& tokens) {
	// Need at least channel + user.
	if (tokens.size() < 2) {
		send(clientSocket, ERR_MSG_INVALID_COMMAND, std::string(ERR_MSG_INVALID_COMMAND).size(), 0);
		return;
	}
	std::string channelName = tokens[0];
	std::string userName    = tokens[1];
	// Build optional reason from any remaining tokens.
	std::string reason = (tokens.size() > 2) ? tokens[2] : "Kicked";
	for (size_t i = 3; i < tokens.size(); i++) {
		reason += " " + tokens[i];
	}

	// Confirm channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = "404 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}
	Channel &channel = _channels[channelName];

	// Confirm user is an operator in the channel.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = "482 " + channelName + " :You're not channel operator\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Look up the user’s FD by nickname.
	int userFd = _findClientFromNickname(userName);
	if (userFd == -1 || std::find(channel.GetUsers().begin(), channel.GetUsers().end(), userFd) == channel.GetUsers().end()) {
		std::string err = "441 " + userName + " :They aren't in the channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	// Notify the kicked user.
	std::string kickMsg = ":" + _clients[clientSocket].GetNickName() + " KICK " + channelName + " " + userName + " :" + reason + "\r\n";
	send(userFd, kickMsg.c_str(), kickMsg.size(), 0);

	// Remove the user from the channel.
	channel.RemoveUser(userFd);

	// Broadcast to remaining channel members.
	for (int memberFd : channel.GetUsers()) {
		send(memberFd, kickMsg.c_str(), kickMsg.size(), 0);
	}
}

// invites a user to a channel
void Server::Invite(int clientSocket, const std::vector<std::string>& tokens) {
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
void Server::Topic(int clientSocket, const std::vector<std::string>& tokens) {
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
void Server::Mode(int clientSocket, const std::vector<std::string>& tokens) {
	// Expected command format: MODE <channel> <mode> [parameters...]
	if (tokens.size() < 2) {
		std::string err = "461 MODE :Not enough parameters\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	std::string channelName = tokens[0];
	std::string modeStr = tokens[1];

	// Check whether channel exists.
	if (_channels.find(channelName) == _channels.end()) {
		std::string err = "403 " + channelName + " :No such channel\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		return;
	}

	Channel &channel = _channels[channelName];
	// Check whether client is a channel operator.
	if (std::find(channel.GetOperators().begin(), channel.GetOperators().end(), clientSocket) == channel.GetOperators().end()) {
		std::string err = "482 " + channelName + " :You're not channel operator\r\n";
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
				std::string err = "421 MODE " + modeStr + " :Unknown MODE command\r\n";
				send(clientSocket, err.c_str(), err.size(), 0);
				return;
			}
		}

		// After a successful mode change, inform all users in the channel.
		std::string response = "MODE " + channelName + " " + modeStr;
		if (tokens.size() > 2) {
			for (size_t i = 2; i < tokens.size(); i++) {
				response += " " + tokens[i];
			}
		}
		response += "\r\n";
		_BroadcastToChannel(channelName, response);

	} catch (std::exception &e) {
		std::string err = "461 MODE " + channelName + " :" + e.what() + "\r\n";
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

// finds a user from a nickname
int Server::_findClientFromNickname(std::string nickname) {
	for (auto it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.GetNickName() == nickname) {
			return it->first;
		}
	}
	return -1;
}

void Server::_changeInviteOnlyRestriction(std::string channel, bool flag) {
    _channels[channel].SetInviteOnly(flag);
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
