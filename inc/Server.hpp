//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_SERVER_H
#define IRC_SERVER_H


class Server {
	public:
		Server(uint16_t port, std::string password);
		~Server();

		void run();
	private:
		uint16_t _port;
		std::string _password;
		int _socket;
		std::vector<pollfd> _pollFds;
};


#endif //IRC_SERVER_H
