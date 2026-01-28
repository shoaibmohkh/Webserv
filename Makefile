NAME = webserv
CXX = c++
RM  = rm -rf
OBJ_FILE = obj

GREEN = \033[0;32m
RED   = \033[0;31m
RESET = \033[0m
ARROW = ✔

INCLUDES = src/Logger \
           src/Client \
           src/Server \

CXXFLAGS = -Wall -Werror -Wextra  -std=c++98 $(addprefix -I, $(INCLUDES))

LOGGER = src/Logger/Logger
Client = src/Client
Server = src/Server

HEADERS =  $(LOGGER).hpp \
           $(Client).hpp \
           $(Server).hpp \

          
TEST = src/testfile

SRCS = $(LOGGER).cpp \
       $(Client).cpp \
       $(WebServer).cpp \


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
