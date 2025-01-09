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
}
