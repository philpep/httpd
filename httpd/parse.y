/*
 * Copyright (c) 2010 Philippe Pepiot <phil@philpep.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Special thanks to Gille Chehade
 * host_v4, host_v6, host_dns, host and interface functions
 * are taken from the OpenSMTPD project.
 * http://www.openbsd.org/cgi-bin/cvsweb/src/usr.sbin/smtpd/
 */

%{
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

#include "yystype.h"
#include "parse.h"
#include "stack.h"
#include "httpd.h"

struct listener *host_v4(const char *, in_port_t);
struct listener *host_v6(const char *, in_port_t);
int host_dns(const char *, in_port_t);
int host(const char *, in_port_t);
int interface(const char *, in_port_t);

static int  yyparse(void);
static int  yyerror(const char *, ...);

/* variables */
YYSTYPE yylval;

%}

%token LISTEN ON ALL PORT
%token HOST ROOT LF
%token <v.s> STRING
%token <v.n> NUMBER

%type <v.n> port
%type <v.s> on

%%
grammar : /* empty */
		| grammar LF
		| grammar main LF
		| grammar host LF
		;

port	: PORT STRING {
			struct servent *servent;
			if (!(servent = getservbyname($2, "tcp"))) {
				yyerror("port %s is invalid", $2);
				YYERROR;
			}
			$$ = servent->s_port;
		}
		| PORT NUMBER {
			if ($2 < 0 || $2 >= (int)USHRT_MAX) {
				yyerror("port %lld is invalid", $2);
				YYERROR;
			}
			$$ = htons($2);
		}
		| /* empty */ {
			$$ = htons(80);
		}
		;

main	: LISTEN on port {
	  		if (!interface($2, $3)) {
				if(host($2, $3) <= 0) {
					yyerror("invalid virtual ip or interface: %s", $2);
					YYERROR;
				}
			}
		}
		;

on		: ON STRING {
			$$ = $2;
		}
		| ON ALL {
			$$ = NULL;
		}
		| /* empty */ {
			$$ = NULL;
		}
		;

host	: HOST STRING ROOT STRING /* TODO listening on specific addr */
	 	{
			struct stat st;
			struct vhost *vh;

			if (stat($4, &st) == -1) {
				yyerror("%s: %s", $4, strerror(errno));
				YYERROR;
			}
			if (!S_ISDIR(st.st_mode)) {
				yyerror("%s: not a directory", $4);
				YYERROR;
			}
			XMALLOC(vh, sizeof(*vh));
			vh->host = $2;
			vh->root = $4;
			TAILQ_INSERT_HEAD(&conf.vhosts, vh, entry);
		}
		;

%%

int
yyerror(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printf("%s:%d: ", file.name, file.lineno);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
	file.error = 1;
    return 0;
}

int
parse_config(const char *filename)
{
	int fd;

	if ((fd = open(filename, O_RDONLY)) == -1)
		err(1, "%s", filename);

	/* init conf */
	TAILQ_INIT(&conf.list);
	TAILQ_INIT(&conf.vhosts);

	file.name = filename;
	file.lineno = 1;
	file.error = 0;

	if (close(STDIN_FILENO) == -1)
		err(1, "stdin");

	if (dup(fd) == -1)
		err(1, "%s", filename); 

	yyparse();
	close(fd);

	return file.error;
}

struct listener *
host_v4(const char *s, in_port_t port)
{
	struct in_addr		 ina;
	struct sockaddr_in	*sain;
	struct listener		*h;

	bzero(&ina, sizeof(ina));
	if (inet_pton(AF_INET, s, &ina) != 1)
		return (NULL);

	XCALLOC(h, 1, sizeof(*h));
	sain = (struct sockaddr_in *)&h->ss;
#if ! defined (__linux__)
	sain->sin_len = sizeof(struct sockaddr_in);
#endif
	sain->sin_family = AF_INET;
	sain->sin_addr.s_addr = ina.s_addr;
	sain->sin_port = port;

	return (h);
}

struct listener *
host_v6(const char *s, in_port_t port)
{
	struct in6_addr		 ina6;
	struct sockaddr_in6	*sin6;
	struct listener		*h;

	bzero(&ina6, sizeof(ina6));
	if (inet_pton(AF_INET6, s, &ina6) != 1)
		return (NULL);

	XCALLOC(h, 1, sizeof(*h));
	sin6 = (struct sockaddr_in6 *)&h->ss;
#if ! defined (__linux__)
	sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = port;
	memcpy(&sin6->sin6_addr, &ina6, sizeof(ina6));

	return (h);
}

int
host_dns(const char *s, in_port_t port)
{
	struct addrinfo		 hints, *res0, *res;
	int			 error, cnt = 0;
	struct sockaddr_in	*sain;
	struct sockaddr_in6	*sin6;
	struct listener		*h;

	bzero(&hints, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM; /* DUMMY */
	error = getaddrinfo(s, NULL, &hints, &res0);
	if (error == EAI_AGAIN || error == EAI_NONAME)
		return (0);
	if (error) {
		warnx("host_dns: could not parse \"%s\": %s", s,
		    gai_strerror(error));
		return (-1);
	}

	for (res = res0; res; res = res->ai_next) {
		if (res->ai_family != AF_INET &&
		    res->ai_family != AF_INET6)
			continue;
		XCALLOC(h, 1, sizeof(*h));
		h->port = port;
		h->ss.ss_family = res->ai_family;
		if (res->ai_family == AF_INET) {
			sain = (struct sockaddr_in *)&h->ss;
#if ! defined (__linux__)
			sain->sin_len = sizeof(struct sockaddr_in);
#endif
			sain->sin_addr.s_addr = ((struct sockaddr_in *)
			    res->ai_addr)->sin_addr.s_addr;
			sain->sin_port = port;
		} else {
			sin6 = (struct sockaddr_in6 *)&h->ss;
#if ! defined (__linux__)
			sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
			memcpy(&sin6->sin6_addr, &((struct sockaddr_in6 *)
			    res->ai_addr)->sin6_addr, sizeof(struct in6_addr));
			sin6->sin6_port = port;
		}

		TAILQ_INSERT_HEAD(&conf.list, h, entry);
		cnt++;
	}
	freeaddrinfo(res0);
	return (cnt);
}

int
host(const char *s, in_port_t port)
{
	struct listener *h;

	h = host_v4(s, port);

	/* IPv6 address? */
	if (h == NULL)
		h = host_v6(s, port);

	if (h != NULL) {
		h->port = port;
		TAILQ_INSERT_HEAD(&conf.list, h, entry);
		return (1);
	}

	return (host_dns(s, port));
}

int
interface(const char *s, in_port_t port)
{
	struct ifaddrs *ifap, *p;
	struct sockaddr_in	*sain;
	struct sockaddr_in6	*sin6;
	struct listener		*h;
	int ret = 0;

	if (getifaddrs(&ifap) == -1)
		err(-1, "getifaddrs");

	for (p = ifap; p != NULL; p = p->ifa_next) {
		if (s && strcmp(s, p->ifa_name) != 0)
			continue;

		XCALLOC(h, 1, sizeof(*h));

		switch (p->ifa_addr->sa_family) {
			case AF_INET:
				sain = (struct sockaddr_in *)&h->ss;
				*sain = *(struct sockaddr_in *)p->ifa_addr;
#if ! defined (__linux__)
				sain->sin_len = sizeof(struct sockaddr_in);
#endif
				sain->sin_port = port;
				break;

			case AF_INET6:
				sin6 = (struct sockaddr_in6 *)&h->ss;
				*sin6 = *(struct sockaddr_in6 *)p->ifa_addr;
#if ! defined (__linux__)
				sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
				sin6->sin6_port = port;
				break;

			default:
				continue;
		}

		h->fd = -1;
		h->port = port;
		ret = 1;
		TAILQ_INSERT_HEAD(&conf.list, h, entry);
	}

	freeifaddrs(ifap);

	return ret;
}
