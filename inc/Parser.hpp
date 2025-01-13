//
// Created by Jonas Götz on 1/9/25.
//

#ifndef IRC_PARSER_H
#define IRC_PARSER_H

#include <string>
#include <vector>
#include "Client.hpp"
#include "Enums.hpp"
#include <tuple>
#include <algorithm>
#include <sstream>

class Parser {
	public:
		Parser();
		~Parser();

		std::tuple<Method, std::vector<std::string>> parse(std::string& msg) const;

	private:
};


#endif //IRC_PARSER_H
