NAME	=	footpaths-processor

RM	=	rm -f

SRCDIR	=	src
INCDIR	=	include

SRC	=	$(wildcard $(SRCDIR)/*.c)

OBJ	=	$(SRC:.c=.o)

INC	=	$(wildcard $(INCDIR)/*.h)

CC	=	gcc

CFLAGS	+=	-I$(INCDIR) \
		-Wall -Werror -W $(shell pkg-config --cflags libxml-2.0)

LDFLAGS	+=	$(shell pkg-config --libs libxml-2.0) -lm

all:		$(NAME)

$(NAME):	$(OBJ)
		$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re:	fclean all

# Compilation database for clangd (Zed, VS Code, vim/nvim, emacs...).
# Regenerate after adding files: `make cdb`
CDB	=	compile_commands.json

cdb:	$(CDB)

$(CDB):	Makefile
	@printf '[\n' > $@
	@first=1; for src in $(SRC); do \
		[ $$first -eq 1 ] || printf ',\n' >> $@; \
		first=0; \
		printf '  {"directory": "%s", "file": "%s", "command": "%s %s -c %s -o %s"}' \
			"$(CURDIR)" "$$src" "$(CC)" "$(CFLAGS)" "$$src" "$${src%.c}.o" >> $@; \
	done
	@printf '\n]\n' >> $@
	@echo "Wrote $@"

.PHONY:	all clean fclean re cdb

%.o: %.c $(INC)
	$(CC) $(CFLAGS) -c -o $@ $<
