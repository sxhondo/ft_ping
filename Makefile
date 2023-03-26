CC 						= gcc
CCFLAGS 				= -Wall -Wextra -Werror -g

NAME 					= ft_ping

INC_DIR 				= ./
SRC_DIR 				= ./
OBJ_DIR 				= obj/

C_SRC 					= ft_ping.c
C_OBJ 					= $(C_SRC:%.c=$(OBJ_DIR)%.o)

CGO_ENABLED				= 0

# ------------------------------------------------------------------------------
#  all
.PHONY: all
all: $(NAME)

# ------------------------------------------------------------------------------
#  build
$(NAME): $(C_OBJ)
	$(CC) $(CCFLAGS) -I $(INC_DIR) $(C_OBJ) -o $@
	sudo setcap cap_net_raw+ep $(NAME)

$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(INC) Makefile
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) -I $(INC_DIR) -c $< -o $@

# ------------------------------------------------------------------------------
#  test
.PHONY: test
test: $(NAME)
	@CGO_ENABLED=$(CGO_ENABLED) go test -v

# ------------------------------------------------------------------------------
#  clean
.PHONY: clean
clean:
	@rm -rf $(OBJ_DIR)

# ------------------------------------------------------------------------------
#  fclean
.PHONY: fclean
fclean: clean
	@rm -f $(NAME)

# ------------------------------------------------------------------------------
#  re
.PHONY: re
re: fclean all
