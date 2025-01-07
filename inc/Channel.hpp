//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include <string>
#include <vector>
#include "Client.hpp"

} Mode;

class Channel {
	public:
		Channel();
		~Channel();

		void AddUser(std::string user);

	private:
		std::string _name;
		std::string _password;
		std::string _topic;
		bool _inviteOnly;
		size_t _userLimit;
		std::vector<std::string> _messages;

		std::vector<Client> _operators;
		std::vector<Client> _users;
};


#endif //IRC_CHANNEL_H
