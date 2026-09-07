NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP
OBJ_DIR = obj
SRCS = $(shell find . -type f -name "*.cpp" -not -path "./$(OBJ_DIR)/*")
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@echo "Linkando o executável $(NAME)..."
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Build concluído com sucesso!"

$(OBJ_DIR)/%.o: %.cpp
	@# Cria o subdiretório correspondente dentro de obj/ antes de compilar
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Removendo a pasta de objetos ($(OBJ_DIR))..."
	rm -rf $(OBJ_DIR)

fclean: clean
	@echo "Removendo o executável $(NAME)..."
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re