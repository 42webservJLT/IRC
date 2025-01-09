//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
	public:
		Channel();
		~Channel();

//		TODO: implement
		std::string GetName() const;
//		std::string GetPassword() const;
		std::string GetTopic() const;
		bool GetInviteOnly() const;
		size_t GetUserLimit() const;
		std::vector<std::string> GetMessages() const;
		std::vector<Client> GetOperators() const;
		std::vector<Client> GetUsers() const;
		std::vector<Client> GetInvited() const;

		void SetName(std::string name);
		void SetPassword(std::string password);
		void SetTopic(std::string topic);
		void SetInviteOnly(bool inviteOnly);
		void SetUserLimit(size_t userLimit);

		void AddUser(std::string user);
		void MakeOperator(std::string user);
		void RemoveOperator(std::string user);
		void RemoveUser(std::string user);
//		TODO: end

	private:
		std::string _name;
		std::string _password;
		std::string _topic;
		bool _inviteOnly;
		bool _topicOnlySettableByOperator;
		size_t _userLimit;
		std::vector<std::string> _messages;

		std::vector<Client> _operators;
		std::vector<Client> _users;
		std::vector<Client> _invited;
};


#endif //IRC_CHANNEL_H
