CC 						= gcc
# CCFLAGS 				= -Wall -Wextra -Werror -g

NAME 					= ft_ping

INC_DIR 				= inc/
SRC_DIR 				= src/
OBJ_DIR 				= obj/

INC 					= $(addprefix $(INC_DIR), ft-ping.h)
C_SRC 					= ft_ping.c
C_OBJ 					= $(C_SRC:%.c=$(OBJ_DIR)%.o)

all: $(NAME)

$(NAME): $(C_OBJ) $(AOBJ)
	$(CC) $(CCFLAGS) -I $(INC_DIR) $(C_OBJ) $(AOBJ) -o $@

$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(INC) Makefile
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) -I $(INC_DIR) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re debug test