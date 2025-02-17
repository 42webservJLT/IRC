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
//		get port & password from command line arguments
		size_t portSizeT = std::stoul(argv[1]);
		if (portSizeT > UINT16_MAX) {
			throw std::out_of_range("Port number out of range for uint16_t");
		}
		uint16_t port = static_cast<uint16_t>(portSizeT);

		std::string password = argv[2];
		if (password.empty()) {
			throw std::invalid_argument("Password cannot be empty");
		}

//		create server instance & set up signal handling
		Server server(port, password);
		Server::SetInstance(&server);
		signal(SIGINT, Server::SignalHandler);

//		run server
		server.Run();
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}