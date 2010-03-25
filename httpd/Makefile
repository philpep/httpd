CC=gcc
YACC=bison
LEX=flex
PROG=httpd
SRC= httpd.c tools.c client.c parse.c token.c
CFLAGS+=-W -Wall -Wextra -g -ggdb3 -D_GNU_SOURCE
LDFLAGS+=-lc -lpthread
OBJ= $(SRC:.c=.o)

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

parse.c: parse.y
	$(YACC) -d -o $@ $<

token.c: token.l
	$(LEX) -t $< > $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o parse.c token.c parse.h $(PROG)
