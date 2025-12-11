NAME = webserv
SRCS = src/testfile.cpp src/config/Tokenizer.cpp src/config/parser.cpp src/Router/Router.cpp
OBJS = $(SRCS:.cpp=.o)
CXX = c++
CXXFLAGS = -Wall -Werror -Wextra --std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re