//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"

Server::Server(uint16_t port, std::string password) : _port(port), _password(password) {
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
	int status = getaddrinfo(server.GetConfig().GetHost().c_str(), std::to_string(server.GetConfig().GetPort()).c_str(), &hints, &res);
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

void Server::Run() {
//	start listening
	if (listen(_socket, SOMAXCONN) < 0) {
		close(_socket);
		throw std::runtime_error("Failed to listen");
	}

//	announce server is running
	std::cout << "Server running on port " << _port << " with password " << _password << std::endl;

//	start polling
	while (true) {
		int pollCount = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollCount < 0) {
			close(_socket);
			throw std::runtime_error("Failed to poll");
		}

		for (auto it = _pollFds.begin(); it != _pollFds.end(); ++it) {
			if (it->revents & POLLIN) {
				if (it->fd == _socket) {
					int clientSocket = accept(_socket, nullptr, nullptr);
					if (clientSocket >= 0) {
						fcntl(clientSocket, F_SETFL, O_NONBLOCK);
						_pollFds.push_back({ clientSocket, POLLIN, 0 });
					}
				} else {
					server.HandleConnection(it->fd);
					close(it->fd);
					it = _pollFds.erase(it);
					if (it == _pollFds.end()) {
						break;
					}
				}
			}
		}
	}
}

void Server::HandleConnection() {
//	TODO: implement
}
