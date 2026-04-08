NAME      = webserv
CXX       = c++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++98

SRC_PATH  = src
OBJ_PATH  = build
INCLUDE   = -Iinclude

SRC_FILES = main \
			configuration \
			server \
			Client \
			HttpRequest

SRC       = $(addprefix $(SRC_PATH)/, $(SRC_FILES:=.cpp))
OBJ       = $(addprefix $(OBJ_PATH)/, $(SRC_FILES:=.o))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ $(OBJ)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.cpp | $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ_PATH):
	@mkdir -p $(OBJ_PATH)

clean:
	@rm -rf $(OBJ_PATH)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
