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

#include "httpd.h"
#include "client.h"
#include "response.h"

struct httpd conf;

static void usage(void);
static void *httpd_accept(void *arg);
static void *serve(void *);

static void
usage(void)
{
	(void)fprintf(stderr, "usage : %s [-hd] -f file\n"
			"\t-h\t\tShow this page\n"
			"\t-d\t\tLaunch daemonized\n"
			"\t-f file\t\tLoad configuration file\n",
			getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int o;
	int daemon = 0;
	pid_t pid;
	struct listener *l;
	char *file = NULL;

	while ((o = getopt(argc, argv, "df:h")) != EOF)
	{
		switch (o)
		{
			case 'd':
				daemon = 1;
				break;
			case 'f':
				file = optarg;
				break;
			case '?':
			case 'h':
				usage();
				break;
		}
	}

	if (!file) {
		warnx("missing config file");
		exit(EXIT_FAILURE);
	}

	/* get config */
	if (parse_config(file) != 0)
		exit(EXIT_FAILURE);

	if (chdir("/") == -1)
		err(1, "/");


	TAILQ_FOREACH(l, &conf.list, entry)
	{
		if ((l->fd = socket(l->ss.ss_family, SOCK_STREAM, 0)) == -1)
		{
			warn("socket");
			l->running = 0;
			continue;
		}
		/* set socket options */
		if (setsockopt(l->fd, SOL_SOCKET, SO_REUSEADDR,
					(int[]){1}, sizeof(int)) == -1) {
			warn("setsockopt");
			l->running = 0;
			continue;
		}
		if (bind(l->fd, (struct sockaddr *)&l->ss, l->ss.ss_len) == -1) {
			warn("bind");
			l->running = 0;
			continue;
		}
		if (listen(l->fd, 5) < 0) {
			warn("listen");
			l->running = 0;
			continue;
		}
		l->running = 1;
	}

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

	TAILQ_FOREACH(l, &conf.list, entry)
	{
		if (l->running && pthread_create(&l->tid, NULL, httpd_accept, (void*)l) != 0)
		{
			warn("pthread_create");
			l->running = 0;
		}
	}

	TAILQ_FOREACH(l, &conf.list, entry)
	{
		if (l->running)
			pthread_join(l->tid, NULL);
	}

	return EXIT_SUCCESS;
}

static void *
httpd_accept(void *arg)
{
	socklen_t len;
	struct Client *c;
	struct listener *l = arg;

	c = client_new();
	for(;;)
	{
		len = sizeof(*l);
		if ((c->fd = accept(l->fd, (struct sockaddr*)&c->ss, &len)) < 0)
			continue;

		if (pthread_create(&c->tid, NULL, serve, (void*)c) != 0)
			warn("pthread_create");
		CLIENT_ADD(c);
		c = client_new();
	}
	return NULL;
}

static void *
serve(void *arg)
{
	if (pthread_detach(pthread_self()) != 0)
		pthread_exit(NULL);

	request_manage(arg);

	return NULL;
}

