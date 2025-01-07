NAME := ircserv

CPP := c++
CPPFLAGS := -Wextra -Wall -Werror -std=c++17 -I./inc

SRCDIR := ./src
OBJDIR = ./obj

SERVERDIR = server

SRC := $(addprefix $(SRCDIR)/$(SERVERDIR)/, Server.cpp)
SRC += $(addprefix $(SRCDIR)/, main.cpp)

OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR)/$(SERVERDIR)
	$(CPP) $(CPPFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

lint:
	cppcheck --error-exitcode=1 --enable=all --suppress=missingInclude ./src
	find ./inc -type f -name "*.hpp" -exec cppcheck --error-exitcode=1 --enable=all --suppress=missingInclude {} \;
