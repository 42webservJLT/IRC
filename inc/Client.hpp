//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H


typedef ClientState enum {
//	client is connected, but not yet authenticated
	CONNECTED,
//	client is connected & authenticated
	AUTHENTICATED,
};

class Client {
	public:
		Client();
		~Client();

	private:
		std::string _userName;
		std::string _nickName;
		std::vector<std::string> _channels;
		bool _authenticated;
		std::vector<std::string> _joinedChannels;
};


#endif //IRC_CLIENT_H
