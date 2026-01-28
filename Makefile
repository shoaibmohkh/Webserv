NAME        := webserv
CXX         := c++
RM          := rm -rf
OBJ_DIR     := obj

GREEN := \033[0;32m
RED   := \033[0;31m
RESET := \033[0m
ARROW := ✔

INCLUDES := include \
            include/config_headers \
            include/Router_headers \
            include/HTTP \
            include/HTTP/http10 \
            include/sockets

CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g3 $(addprefix -I, $(INCLUDES))

CONFIG_SRCS := \
	location_parser.cpp \
	parser.cpp \
	server_parser.cpp \
	Tokenizer.cpp

HTTP_SRCS := \
	Http10Parser.cpp \
	Http10Serializer.cpp

SOCKET_SRCS := \
	PollReactor.cpp \
	NetChannel.cpp \
	NetUtil.cpp \
	ListenPort.cpp

ROUTER_SRCS := \
	Router.cpp \
	autoindex.cpp \
	cgi_router.cpp \
	error_page.cpp \
	files_handeling.cpp \
	method_router.cpp \
	router_utils.cpp \
	RouterByteHandler.cpp

MAIN_SRC := main.cpp

SRCS := \
	$(CONFIG_SRCS) \
	$(HTTP_SRCS) \
	$(SOCKET_SRCS) \
	$(ROUTER_SRCS) \
	$(MAIN_SRC)

VPATH := \
	src \
	src/config \
	src/HTTP \
	src/Router \
	src/sockets

OBJS := $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)Done $(ARROW)$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Removing object files...$(RESET)"
	@$(RM) $(OBJ_DIR)
	@echo "$(RED)Done $(ARROW)$(RESET)"

fclean: clean
	@echo "$(RED)Removing executable...$(RESET)"
	@$(RM) $(NAME)
	@echo "$(RED)Done $(ARROW)$(RESET)"

re: fclean all

.PHONY: all clean fclean re
