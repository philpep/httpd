PROG= httpd
SRCS= httpd.c tools.c client.c response.c parse.y
CFLAGS+= -Wall -W -Wextra -g -ggdb3
LDFLAGS+= -lc -lpthread -lmagic
YFLAGS=
NO_MAN= YES


.include <bsd.prog.mk>
