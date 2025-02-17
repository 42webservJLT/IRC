#include "Parser.hpp"
#include <iostream>

Parser::Parser() {

}

Parser::~Parser() {

}

std::tuple<Method, std::vector<std::string>> Parser::parse(const std::string &msg) const {
	std::vector<std::string> tokens;
	std::istringstream tokenStream(msg);
	std::string token;

	// Split on whitespace, except if token starts with ':'
	while (tokenStream >> token) {
		if (!token.empty() && token[0] == ':') {
			// Everything else goes into one trailing token
			std::string trailingParam = token.substr(1);
			std::string rest;
			std::getline(tokenStream, rest);
			if (!rest.empty())
				trailingParam += rest;
			tokens.push_back(trailingParam);
			break;
		}
		tokens.push_back(token);
	}

	std::cout << "Tokens: ";
	for (const std::string &t : tokens) {
		std::cout << t << " ";
	}
	std::cout << std::endl;
	// Determine the command
	Method method = INVALID;
	if (!tokens.empty()) {
		std::string command = tokens[0];
		std::transform(command.begin(), command.end(), command.begin(), ::toupper);
		if (command == "PASS")     method = AUTHENTICATE;
		else if (command == "NICK") method = NICK;
		else if (command == "USER") method = USER;
		else if (command == "JOIN") method = JOIN;
		else if (command == "PRIVMSG") method = MSG;
		else if (command == "KICK") method = KICK;
		else if (command == "INVITE") method = INVITE;
		else if (command == "TOPIC")  method = TOPIC;
		else if (command == "MODE")   method = MODE;
		else if (command == "PING")   method = PING;
		else if (command == "QUIT")   method = QUIT;
		else                          method = INVALID;
	}
	std::cout << "Method: " << method << std::endl;

	// Remove the first token (the command) to leave only parameters
	std::vector<std::string> params;
	if (tokens.size() > 0) {
		params.assign(tokens.begin() + 1, tokens.end());
	}
	return std::make_tuple(method, params);
}
