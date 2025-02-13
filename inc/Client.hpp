//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H

#include <string>
#include <vector>
#include "Enums.hpp"

class Client {
	public:
		Client();
		Client(int fd);
		~Client();

		std::string GetUserName() const;
		std::string GetNickName() const;
		bool GetAuthenticated() const;
		std::string& GetMsgBuffer();
		int GetFd() const;

		void SetUserName(std::string userName);
		void SetNickName(std::string nickName);
		void SetAuthenticated(bool authenticated);

	private:
		int _fd;
		std::string _userName;
		std::string _nickName;
		bool _authenticated;

//		holds chunked message
		std::string _msgBuffer;
};


#endif //IRC_CLIENT_H
