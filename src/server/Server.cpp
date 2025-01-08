//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"

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
	struct addrinfo* res;
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
	_pollFds.push_back({ _socket, POLLIN, 0 });
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

		if (msg.find("\n") != std::string::npos) {
//			verify authenticated
			if (!_clients[clientSocket].GetAuthenticated()) {
//				TODO: implement
			}
//			TODO: implement
//			parse message
//			handle message
//			send response
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
