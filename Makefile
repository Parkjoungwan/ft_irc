NAME = ircserv

CPP = c++
CPPFLAGS = -std=c++98 -Wall -Wextra -Werror
INC = -Iinclude/

SRC_DIR = src/

SRC_NAME = main.cpp\
			Channel.cpp\
			Client.cpp\
			Server.cpp\
			Command.cpp\
			Util.cpp

SRCS = $(addprefix $(SRC_DIR), $(SRC_NAME))

OBJS = $(SRCS:.cpp=.o)

all : $(NAME)

$(NAME) : $(OBJS)
		$(CPP) $(CPPFLAGS) $(INC) $(OBJS) -o $(NAME)

.cpp.o :
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $(<:.cpp=.o)

clean :
		rm -rf $(OBJS)

fclean :clean
		rm -rf $(NAME)

re : fclean all

.PHONY : all clean fclean re
