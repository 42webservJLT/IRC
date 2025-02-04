//
// Created by Leon David Zipp on 1/10/25.
//

#ifndef IRC_ENUMS_H
#define IRC_ENUMS_H

enum ClientState {
//	client is connected, but not yet authenticated
	CONNECTED,
//	client is connected & authenticated
	AUTHENTICATED,
};

enum Method {
	AUTHENTICATE,
	NICK,
	USER,
	JOIN,
	PRIVMSG,
	KICK,
	INVITE,
	TOPIC,
	MODE,
	PING,
	INVALID,
};

enum Mode {
	MAKE_INVITE_ONLY, // +i
	UNMAKE_INVITE_ONLY, // -i
	MAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR, // +t
	UNMAKE_TOPIC_ONLY_SETTABLE_BY_OPERATOR, // -t
	GIVE_OPERATOR_PRIVILEGES, // +o
	TAKE_OPERATOR_PRIVILEGES, // -o
	SET_USER_LIMIT, // +l <limit>
	UNSET_USER_LIMIT, // -l
	SET_PASSWORD, // +k <password>
	UNSET_PASSWORD, // -k
	INVALID_MODE,
};

#endif //IRC_ENUMS_H
