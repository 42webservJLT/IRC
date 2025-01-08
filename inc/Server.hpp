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
#include <map>
#include "Client.hpp"
#include "Channel.hpp"

#define MAX_BUFFER_SIZE 1024
#define MAX_CONNECTIONS SOMAXCONN

class Server {
	public:
		Server(uint16_t port, std::string password);
		~Server();

//		runs the server
		void Run();

//		connection handling
		void HandleNewConnection();
		void HandleConnection(int clientSocket);
		void HandleDisconnection(int clientSocket);

//		client commands
		void Authenticate(int clientSocket, const std::string& password);
		void Nick(int clientSocket, const std::string& nickname);
		void User(int clientSocket, const std::string& username);
		void Join(int clientSocket, const std::string& channel);
		void PrivMsg(int clientSocket, const std::string& target, const std::string& message);

//		operator commands for channels
		void Kick(int clientSocket, const std::string& channel, const std::string& target);
		void Invite(int clientSocket, const std::string& channel, const std::string& target);
		void Topic(int clientSocket, const std::string& channel, const std::string& topic);
		void Mode(int clientSocket, const std::string& channel, const std::string& mode);

//		getters
		std::string GetHost() const;
		uint16_t GetPort() const;
		std::string GetPassword() const;
		int GetSocket() const;
		std::vector<pollfd> GetPollFds() const;

	private:
		std::string _host;
		uint16_t _port;
		std::string _password;
		int _socket;
		std::vector<pollfd> _pollFds;
//		maps client socket to client
		std::map<int, Client> _clients;
//		maps channel name to channel
		std::map<std::string, Channel> _channels;
};


#endif //IRC_SERVER_H
