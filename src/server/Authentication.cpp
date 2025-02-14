//
// Created by Leon David Zipp on 2/11/25.
//

#include "Server.hpp"
#include "Client.hpp"

// authenticates a client if the password is correct
void Server::Authenticate(int clientSocket, const std::vector<std::string>& tokens) {
	if (tokens.size() != 1 || tokens[0] != GetPassword()) {
		std::cout << "vector client size: " << _clients.size() << std::endl;
		std::string err = "464 " + _clients[clientSocket].GetNickName() + " PASS :Password incorrect\r\n";
		send(clientSocket, err.c_str(), err.size(), 0);
		// PREVIOUS APPROACH: HandleDisconnection(clientSocket); // Disconnect the client on failed authentication
		RemoveClient(clientSocket); // forcibly disconnect
	} else {
		_pw_check = true;
		_clients[clientSocket].SetAuthenticated(true);
		// Optionally, send a welcome message or further instructions
		RegisterClientIfReady(clientSocket);
	}
}

// registers a client if all necessary information is set
void Server::RegisterClientIfReady(int clientSocket) {
	Client &c = _clients[clientSocket];
	if (c.GetAuthenticated() && !c.GetNickName().empty() && !c.GetUserName().empty()) {
		std::string welcome = "001 " + c.GetNickName() + " :Welcome to the IRC Server\r\n";
		send(clientSocket, welcome.c_str(), welcome.size(), 0);
	}
}
