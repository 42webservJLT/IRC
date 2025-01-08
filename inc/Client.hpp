//
// Created by Leon David Zipp on 1/7/25.
//

#ifndef IRC_CLIENT_H
#define IRC_CLIENT_H

#include <string>
#include <vector>


typedef enum {
//	client is connected, but not yet authenticated
	CONNECTED,
//	client is connected & authenticated
	AUTHENTICATED,
} ClientState;

class Client {
	public:
		Client();
		~Client();

		std::string GetUserName() const;
		std::string GetNickName() const;
		bool GetAuthenticated() const;

//		TODO: implement
		void SetUserName(std::string userName);
		void SetNickName(std::string nickName);
		void SetAuthenticated(bool authenticated);
//		TODO: end

	private:
		std::string _userName;
		std::string _nickName;
		bool _authenticated;
};


#endif //IRC_CLIENT_H
