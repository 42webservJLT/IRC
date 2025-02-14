NAME := ircserv

GREEN := "\033[0;32m"
BLUE := "\033[0;34m"
RESET := "\033[0m"

CPP := c++
CPPFLAGS := -Wextra -Wall -Werror -std=c++17 -I./inc
CPPFLAGS += -fsanitize=address -g

SRCDIR := ./src
OBJDIR = ./obj

SERVERDIR = server
CLIENTDIR = client
CHANNELDIR = channel
PARSERDIR = parser

SRC := $(addprefix $(SRCDIR)/$(SERVERDIR)/, \
	Authentication.cpp \
	ClientCommands.cpp \
	Connections.cpp \
	Helpers.cpp \
	ModeCommand.cpp \
	OperatorCommands.cpp \
	Server.cpp)
SRC += $(addprefix $(SRCDIR)/$(CHANNELDIR)/, Channel.cpp)
SRC += $(addprefix $(SRCDIR)/$(CLIENTDIR)/, Client.cpp)
SRC += $(addprefix $(SRCDIR)/$(PARSERDIR)/, Parser.cpp)
SRC += $(addprefix $(SRCDIR)/, main.cpp)

OBJ := $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)/$(SERVERDIR)
	@mkdir -p $(OBJDIR)/$(CLIENTDIR)
	@mkdir -p $(OBJDIR)/$(CHANNELDIR)
	@mkdir -p $(OBJDIR)/$(PARSERDIR)
	@$(CPP) $(CPPFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJ)
	@echo $(BLUE)"Compiling IRC server..."$(RESET)
	@$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)
	@echo $(GREEN)"IRC server compiled!"$(RESET)

clean:
	@echo $(BLUE)"Cleaning object files..."$(RESET)
	@rm -rf $(OBJDIR)
	@echo $(GREEN)"Object files cleaned!"$(RESET)

fclean: clean
	@rm -f $(NAME)

re: fclean all

start:
	@docker compose up --remove-orphans -d
	@make server

stop:
	@docker compose down

server:
	@./ircserv 6667 abc

client:
	@docker exec -it weechat weechat

client2:
	@docker exec -it weechat2 weechat

client3:
	@docker exec -it weechat3 weechat

netcat:
	@docker exec -it netcat nc -C host.docker.internal 6667

lint:
	@cppcheck --error-exitcode=1 --enable=all --suppress=missingInclude ./src
	@find ./inc -type f -name "*.hpp" -exec cppcheck --error-exitcode=1 --enable=all --suppress=missingInclude {} \;

.PHONY: all clean fclean re start server client lint
