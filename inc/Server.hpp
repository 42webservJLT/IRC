//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_SERVER_H
#define IRC_SERVER_H

#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>


class Server {
	public:
		Server(uint16_t port, std::string password);
		~Server();

		void Run();
		void HandleConnection(int clientSocket);
		uint16_t GetPort() const;
		std::string GetPassword() const;
		int GetSocket() const;
		std::vector<pollfd> GetPollFds() const;
	private:
		uint16_t _port;
		std::string _password;
		int _socket;
		std::vector<pollfd> _pollFds;
};


#endif //IRC_SERVER_H
