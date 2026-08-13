NAME	=	footpaths-processor

RM	=	rm -f

SRC	=	main.c

OBJ	=	$(SRC:.c=.o)

GCC	=	gcc

CFLAGS	+=	-I./include \
		-Wall -Werror -W

all:		$(NAME)

$(NAME):	$(OBJ)
		$(GCC) $(CFLAGS) $? $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re:	fclean all

.PHONY:	all clean fclean re

%.o: %.c ./include/my.h
	$(CC) $(CFLAGS) -c -o $@ $<
