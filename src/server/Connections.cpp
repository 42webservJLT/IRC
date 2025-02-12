//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"
#include "Client.hpp"

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
}

// handles a connection
void Server::HandleConnection(int clientSocket) {
	char buffer[MAX_BUFFER_SIZE + 1];
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

// removes a client from the server
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

// handles a clients interactions with the server
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
