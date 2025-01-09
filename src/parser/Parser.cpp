#include "Parser.hpp"

Parser::Parser() {

}

Parser::~Parser() {

}

std::tuple<Method, std::vector<std::string>&> Parser::parse(std::string& msg) const {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(msg);
    Method method = INVALID;
    // Get first token (command)
    if (std::getline(tokenStream, token, ' ')) {
        // Convert to uppercase for case-insensitive comparison
        std::transform(token.begin(), token.end(), token.begin(), ::toupper);

        if (token == "AUTHENTICATE") method = AUTHENTICATE;
        else if (token == "NICK") method = NICK;
        else if (token == "USER") method = USER;
        else if (token == "JOIN") method = JOIN;
        else if (token == "PRIVMSG") method = PRIVMSG;
        else if (token == "KICK") method = KICK;
        else if (token == "INVITE") method = INVITE;
        else if (token == "TOPIC") method = TOPIC;
        else if (token == "MODE") method = MODE;
        else method = INVALID;
}
