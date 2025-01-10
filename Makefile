NAME := ircserv

CPP := c++
CPPFLAGS := -Wextra -Wall -Werror -std=c++17 -I./inc

SRCDIR := ./src
OBJDIR = ./obj

SERVERDIR = server
CLIENTDIR = client
CHANNELDIR = channel
PARSERDIR = parser

SRC := $(addprefix $(SRCDIR)/$(SERVERDIR)/, Server.cpp)
SRC += $(addprefix $(SRCDIR)/$(CHANNELDIR)/, Channel.cpp)
SRC += $(addprefix $(SRCDIR)/$(CLIENTDIR)/, Client.cpp)
SRC += $(addprefix $(SRCDIR)/$(PARSERDIR)/, Parser.cpp)
SRC += $(addprefix $(SRCDIR)/, main.cpp)

OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR)/$(SERVERDIR)
	mkdir -p $(OBJDIR)/$(CLIENTDIR)
	mkdir -p $(OBJDIR)/$(CHANNELDIR)
	mkdir -p $(OBJDIR)/$(PARSERDIR)
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

