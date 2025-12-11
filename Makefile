NAME = webserv
SRCS = src/testfile.cpp \
       src/config/Tokenizer.cpp \
       src/config/parser.cpp \
	   src/config/server_parser.cpp \
	   src/config/location_parser.cpp \
	   src/Router/Router.cpp \
	   src/Router/router_utils.cpp \
	   src/Router/method_router.cpp \
	   src/Router/files_handeling.cpp \
	   src/Router/cgi_router.cpp \
	   src/Router/autoindex.cpp \

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