//
// Created by Jonas Götz on 1/9/25.
//

#ifndef IRC_PARSER_H
#define IRC_PARSER_H

#include <string>
#include <vector>
#include "Client.hpp"
#include "Enums.hpp"

class Parser {
	public:
		Parser();
		~Parser();

//		TODO: implement
		std::tuple<Method, std::vector<std::string>&> parse(std::string& msg) const;
//		TODO: end

	private:
};


#endif //IRC_PARSER_H
