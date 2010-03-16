/*
 *   Copyright (C) 2009 Philippe Pepiot <phil@philpep.org>
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *	 copyright notice, this list of conditions and the following disclaimer
 *	 in the documentation and/or other materials provided with the
 *	 distribution.
 *   * Neither the name of the  nor the names of its
 *	 contributors may be used to endorse or promote products derived from
 *	 this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

