//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H


typedef ClientState enum {
	CONNECTED,
	REGISTERED,
	AWAY,
	OPERATOR
};

class Client {
	public:
		Client();
		~Client();

	private:
		std::string _userName;
		std::string _nickName;
		std::vector<std::string> _channels;
		ClientState _state;
};


#endif //IRC_CLIENT_H
