#include "Parser.hpp"

Parser::Parser() {

}

Parser::~Parser() {

}

std::tuple<Method, std::vector<std::string>> Parser::parse(std::string& msg) const {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(msg);
	Method method = INVALID;

	// Get first token (command)
	if (std::getline(tokenStream, token, ' ')) {
		// Convert to uppercase for case-insensitive comparison
		std::transform(token.begin(), token.end(), token.begin(), ::toupper);

		if (token == "PASS") method = AUTHENTICATE;
		else if (token == "NICK") method = NICK;
		else if (token == "USER") method = USER;
		else if (token == "JOIN") method = JOIN;
		else if (token == "PRIVMSG") method = PRIVMSG;
		else if (token == "KICK") method = KICK;
		else if (token == "INVITE") method = INVITE;
		else if (token == "TOPIC") method = TOPIC;
		else if (token == "MODE") method = MODE;
		else if (token == "PING") method = PING;
		else if (token == "QUIT") method = QUIT;
		// tokens that are not implemented becuase the subject doesn't require them. can ignore or reply with 421 Unknown command
		else if (token == "PART" || token == "LIST" || token == "NAMES" || token == "WHO" || token == "WHOIS" || token == "WHOWAS" || token == "MOTD" || token == "LUSERS" || token == "VERSION" || token == "STATS" || token == "LINKS" || token == "TIME" || token == "CONNECT" || token == "TRACE" || token == "ADMIN" || token == "INFO" || token == "SERVLIST" || token == "SQUERY" || token == "SILENCE" || token == "REHASH" || token == "DIE" || token == "RESTART" || token == "SUMMON" || token == "USERS" || token == "WALLOPS" || token == "USERHOST" || token == "ISON") method = INVALID;
		else method = INVALID;
	}

	while (std::getline(tokenStream, token, ' ')) {
		if (!token.empty()) {
			tokens.push_back(token);
		}
	}

	return std::make_tuple(method, tokens);
}
