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

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/socket.h>

#include "client.h"
#include "response.h"

/* top of the client slist */
static void *serve(void *);

int
main(int argc, char *argv[])
{
	int o;
	int daemon = 0;
	struct addrinfo hint, *res;
	int tmp;
	int fd; /* server socket */
	struct sockaddr_storage srv_addr;
	pid_t pid;
	struct Client *c;

	while ((o = getopt(argc, argv, "dh")) != EOF)
	{
		switch (o)
		{
			case 'd':
				daemon = 1;
				break;
			case '?':
			case 'h':
				errx(EXIT_FAILURE, "usage : httpd [-d]");
				break;
		}
	}

	warnx("Server listen on %s:%s root %s", "*", "8081", ".");


	bzero(&hint, sizeof(hint));
	hint.ai_flags = AI_PASSIVE;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = PF_INET;

	if (NULL /* TODO CONF HOST */)
		hint.ai_flags |= AI_CANONNAME;

	if ((tmp = getaddrinfo(NULL/* TODO CONF HOST */, "8080" /* TODO CONF PORT */, &hint, &res)))
		errx(EXIT_FAILURE, "getaddrinfo : %s", gai_strerror(tmp));


	if ((fd = socket(res->ai_family,
					res->ai_socktype,
					res->ai_protocol)) == -1)
		err(EXIT_FAILURE, "socket");

	memcpy(&srv_addr, res->ai_addr, res->ai_addrlen);

	/* set socket options */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
				(int[]){1}, sizeof(int)) == -1)
		err(EXIT_FAILURE, "setsockopt");

	if (bind(fd, (struct sockaddr *)&srv_addr,
				res->ai_addrlen) == -1)
		err(EXIT_FAILURE, "bind");

	if (listen(fd, 5) < 0)
		err(EXIT_FAILURE, "listen");

	/* daemonize */
	if (daemon && getppid() != 1)
	{
		pid = fork();

		if (pid < 0)
			err(EXIT_FAILURE, "fork");

		if (pid > 0)
			exit(EXIT_SUCCESS);

		umask(0222);

		if (setsid() < 0)
			err(EXIT_FAILURE, "setsid");

		freopen("/dev/null", "r", stdin);
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
	}

	SLIST_INIT(&clients);
	c = client_new();
	socklen_t sin_size;

	/* main loop */
	for (;;)
	{
		sin_size = sizeof(struct sockaddr_in);

		if ((c->fd = accept(fd, (struct sockaddr*)&c->sin, &sin_size)) < 0)
			continue;

		/* Reflexion : fatal or not ? */
		if (pthread_create(&c->tid, NULL, serve, (void*)c) != 0)
			warn("pthread_create");

		//warnx("%s:%d on socket %d", inet_ntoa(c->sin.sin_addr),
		//htons(c->sin.sin_port), c->fd);

		CLIENT_ADD(c);
		c = client_new();
	}

	return EXIT_SUCCESS;
}

static void *
serve(void *arg)
{
	if (pthread_detach(pthread_self()) != 0)
		pthread_exit(NULL);

	request_manage(arg);

	return NULL;
}

