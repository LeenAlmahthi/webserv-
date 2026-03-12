NAME      = webserv
CXX       = c++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++98
INCLUDES  = -Iinclude

SRC_PATH  = src
OBJ_PATH  = build

SRC_FILES = main configuration server

SRC       = $(addprefix $(SRC_PATH)/, $(addsuffix .cpp, $(SRC_FILES)))
OBJ       = $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(SRC_FILES)))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJ)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.cpp | $(OBJ_PATH)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_PATH):
	mkdir -p $(OBJ_PATH)

clean:
	rm -rf $(OBJ_PATH)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re