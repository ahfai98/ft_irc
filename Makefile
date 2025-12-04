
NAME = ircserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -Iinc

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = inc

SRCS = main.cpp \
       Server.cpp Client.cpp Channel.cpp \
       pass.cpp nick.cpp user.cpp \
	   join.cpp part.cpp \
       privmsg.cpp mode.cpp quit.cpp topic.cpp \
	   ping.cpp invite.cpp kick.cpp

SRC_FILES = $(addprefix $(SRC_DIR)/,$(SRCS))

OBJS = $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))

DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "Build complete!"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

-include $(DEPS)

clean:
	@rm -rf $(OBJ_DIR)
	@echo "Cleaned object files."

fclean: clean
	@rm -f $(NAME)
	@echo "Cleaned executable."

re: fclean all

.PHONY: all clean fclean re
