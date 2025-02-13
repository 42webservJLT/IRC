//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CHANNEL_H
#define IRC_CHANNEL_H

#include <string>
#include <vector>
#include <algorithm>
#include "Enums.hpp"
#include "Client.hpp"

class Channel {
	public:
		Channel();
		~Channel();

//		TODO: implement
		std::string GetName() const;
		std::string GetTopic() const;
		bool GetInviteOnly() const;
		size_t GetUserLimit() const;
		std::vector<std::string>& GetMessages();
		std::vector<int>& GetOperators();
		std::vector<int>& GetUsers();
		std::vector<int>& GetInvited();
		std::string GetPassword() const; // (from channel.cpp)

		void SetName(std::string name);
		void SetPassword(std::string password);
		void SetTopicOnlySettableByOperator(bool topicOnlySettableByOperator);
		void SetTopic(std::string topic);
		void SetInviteOnly(bool inviteOnly);
		void SetUserLimit(size_t userLimit);
		void SetTopicSetBy(const std::string &nick);
		const std::string& GetTopicSetBy() const;
		time_t GetTopicSetTime() const;
		void SetTopicSetTime(time_t time);


		void AddUser(int user);
		void MakeOperator(int user);
		void RemoveOperator(int user);
		void RemoveUser(int user);
		bool IsUserOperator(int user);
//		TODO: end

	private:
		std::string _name;
		std::string _password;
		bool _inviteOnly;
		size_t _userLimit;
		std::vector<std::string> _messages;
		
		
		std::string _topic;          // Text content of the topic
    	std::string _topicSetBy;     // Nick of who set the topic
		bool _topicOnlySettableByOperator; // Whether only operators can set the topic
    	time_t _topicSetTime;        // When the topic was set

//		use the fd of the client instead of the client itself
		std::vector<int> _operators;
		std::vector<int> _users;
		std::vector<int> _invited;
};


#endif //IRC_CHANNEL_H
