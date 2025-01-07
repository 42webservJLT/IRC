//
// Created by Leon David Zipp on 1/7/25.
//

#include "main.hpp"

int main(int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	try {
		size_t port = std::stoi(argv[1]);
		std::string password = argv[2];
	} catch (std::exception &e) {
		std::cerr << "Invalid arguments: " << e.what() << std::endl;
		return 1;
	}

	Server server(port, password);
	server.run();

	return 0;
}