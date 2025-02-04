//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_SERVER_H
#define IRC_SERVER_H

#include <sys/socket.h>
#include <string>
#include <poll.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <map>
#include <algorithm>
#include "Client.hpp"
#include "Channel.hpp"
#include "Parser.hpp"
#include "Enums.hpp"

#define MAX_BUFFER_SIZE 1024
#define MAX_CONNECTIONS SOMAXCONN
#define NO_USER_LIMIT 0

#define ERR_MSG_UNAUTHENTICATED "You are not authenticated yet\n"
#define ERR_MSG_INVALID_COMMAND "Invalid command\n"
#define ERR_MSG_CHANNEL_NOT_FOUND "Channel not found\n"
#define ERR_MSG_CHANNEL_NOT_INVITED "You are not invited to this channel\n"
#define ERR_MSG_CHANNEL_FULL "Channel is full\n"
#define ERR_MSG_NOT_A_CHANNEL_OPERATOR "You are not a channel operator\n"
#define ERR_MSG_USER_NOT_FOUND "User not found\n"
#define ERR_MSG_USER_NOT_IN_CHANNEL "User is not in channel\n"

#define INVITED_MSG(channel, user) "You have been invited to channel " + channel + "\n"

class Server {
	public:
		Server(uint16_t port, std::string password);
		~Server();

//		runs the server
		bool Run();

//		connection handling
		void HandleNewConnection();
		void HandleConnection(int clientSocket);
		void HandleDisconnection(int clientSocket);
		void Ping(int clientFd, const std::vector<std::string>& tokens);
		void RemoveClient(int clientFd);
		bool HandleClient(int clientFd);
//		verification methods
		void Authenticate(int clientSocket, const std::vector<std::string>& tokens);
		void RegisterClientIfReady(int clientSocket);

//		client commands
		void Nick(int clientSocket, const std::vector<std::string>& tokens);
		void User(int clientSocket, const std::vector<std::string>& tokens);
		void Join(int clientSocket, const std::vector<std::string>& tokens);
		void PrivMsg(int clientSocket, const std::vector<std::string>& tokens);
		void Quit(int clientSocket, const std::vector<std::string>& tokens);

//		operator commands for channels
		void Kick(int clientSocket, const std::vector<std::string>& tokens);
		void Invite(int clientSocket, const std::vector<std::string>& tokens);
		void Topic(int clientSocket, const std::vector<std::string>& tokens);
		void Mode(int clientSocket, const std::vector<std::string>& tokens);

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
//		mapping of method to function
		std::map<Method, void (Server::*)(int, const std::vector<std::string>&)> _methods;
//		instance of parser class
		Parser _parser;
		int _listeningFd;

//		mode sub commands
		void _changeInviteOnlyRestriction(std::string channel, bool isInviteOnly);
		void _changeTopicRestriction(std::string channel, bool isTopicOnlySettableByOperator);
		void _changePasswordRestriction(std::string channel, std::string password);
		void _changeOperatorPrivileges(std::string channel, std::string user, bool isOperator);
		void _changeUserLimitRestriction(std::string channel, size_t userLimit);
		int _findClientFromNickname(std::string nickname);
		void _BroadcastToChannel(const std::string &channelName, const std::string &msg);
};


#endif //IRC_SERVER_H
