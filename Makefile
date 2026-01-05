NAME = webserv

CXX = c++
RM  = rm -rf
OBJ_FILE = obj

GREEN = \033[0;32m
RED   = \033[0;31m
RESET = \033[0m
ARROW = ✔

INCLUDES = include \
           include/config_headers \
           include/Router_headers

CXXFLAGS = -Wall -Werror -Wextra -std=c++98 $(addprefix -I, $(INCLUDES))


CONFIG_HEADERS = include/config_headers/Config.hpp \
                 include/config_headers/Parser.hpp \
                 include/config_headers/Tokenizer.hpp

ROUTER_HEADERS = include/Router_headers/Router.hpp

HEADERS = $(CONFIG_HEADERS) $(ROUTER_HEADERS)

HTTP = 

TEST = src/testfile.cpp

CONFIG = src/config/location_parser.cpp \
         src/config/parser.cpp \
         src/config/server_parser.cpp \
         src/config/Tokenizer.cpp

ROUTER = src/Router/Router.cpp \
         src/Router/autoindex.cpp \
         src/Router/cgi_router.cpp \
         src/Router/error_page.cpp \
         src/Router/files_handeling.cpp \
         src/Router/method_router.cpp \
         src/Router/router_utils.cpp

SRCS = $(TEST) $(CONFIG) $(ROUTER)

OBJS = $(addprefix $(OBJ_FILE)/, $(SRCS:.cpp=.o))


all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Making $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)Done $(ARROW)$(RESET)"

$(OBJ_FILE)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Deleting $(OBJ_FILE)...$(RESET)"
	@$(RM) $(OBJ_FILE)
	@echo "$(RED)Done $(ARROW)$(RESET)"

fclean: clean
	@echo "$(RED)Deleting $(NAME)...$(RESET)"
	@$(RM) $(NAME)
	@echo "$(RED)Done $(ARROW)$(RESET)"

re: fclean all

.PHONY: all clean fclean re
