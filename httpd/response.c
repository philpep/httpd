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
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <magic.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "response.h"

#define INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\r\n" \
	"Connection: close\r\n" \
	"Server: OpenHTTPD/1.0\r\n\r\n"


static void send_error(struct Client *c);
static void send_uri(struct Client *c);
static void header_send(struct Client *c);
static void header_set(struct Client *c, const char *key, const char *fmt, ...);
static char *header_get(struct Client *c, const char *key);
static char *status_get(int code);


static struct st_code status_code[] = {
#include "status_code.h"
};

void
request_manage(struct Client *c)
{
	char buf[BUFSIZ];
	void *data = NULL;
	ssize_t n;
	size_t nread = 0;
	off_t offset;
	char **hdrs, **line;
	size_t hdrs_size, i;
	struct http_hdrs *hel;	/* header element */

	signal(SIGPIPE, SIG_IGN);

	while ((n = read(c->fd, buf, BUFSIZ)) != -1 && n != 0)
	{
		nread += n;

		XREALLOC(data, nread);
		memcpy(data+nread-n, buf, n);

		offset = (nread-n-4 > 0) ? nread-n-4 : 0;

		if ((c->body = memmem(data+offset, nread-offset, "\r\n\r\n", 4))) {
			*(char*)c->body = '\0';
			c->body += 4;
			c->bsize = c->body - data;
			break;
		}
	}

	if (n == -1 || n == 0)
		client_destroy();

	mstack_push(data);

	do
	{
		c->code = 0;

		if (!(hdrs = splitstr((char*)data, "\r\n", &hdrs_size))
				|| !hdrs[0]) {
			c->code = 400;
			break;
		}
	
		if (!(line = splitstr(hdrs[0], " ", NULL)) || !line[0]
				|| !line[1] || !line[2]) {
			c->code = 400;
			break;
		}
	
		if (!strcmp(line[0], "GET"))
			c->method = GET;
		else if (!strcmp(line[0], "HEAD"))
			c->method = HEAD;
		else if (!strcmp(line[0], "POST"))
			c->method = POST;
		else if (!strcmp(line[0], "OPTIONS"))
			c->method = OPTIONS;
		else if (!strcmp(line[0], "PUT"))
			c->method = PUT;
		else if (!strcmp(line[0], "DELETE"))
			c->method = DELETE;
		else if (!strcmp(line[0], "TRACE"))
			c->method = TRACE;
		else if (!strcmp(line[0], "CONNECT"))
			c->method = CONNECT;
		else {
			c->method = NONE;
			c->code = 400;
			break;
		}
	
		c->uri = line[1];
	
		if (c->uri[0] != '/' &&
				strncmp(c->uri, "http://", 7)) {
			c->code = 400;
			break;
		}
	
		if (!strcmp(line[2], "HTTP/1.1"))
			c->version = HTTP11;
		else if (!strcmp(line[2], "HTTP/1.0"))
			c->version = HTTP10;
		else {
			c->code = (!strncmp(line[2], "HTTP/", 5)) ? 505 : 101;
			break;
		}
	
		for (i = 1; hdrs[i] != NULL; i++)
		{
			line = splitstr(hdrs[i], ": ", NULL);
	
			if (line && line[0] && line[1]) {
				ZMALLOC(hel, sizeof(*hel));
				hel->key = line[0];
				hel->val = line[1];
				SLIST_INSERT_HEAD(&c->reqh, hel, next);
				printf("'%s'\t->\t'%s'\n", line[0], line[1]);
			}
		}
	} while (0);


	if (c->code != 0)
		send_error(c);
	else
		send_uri(c);

	client_destroy();
}

static void
send_error(struct Client *c)
{
	char *msg;
	if (c->code == 101 || c->code == 505)
		header_set(c, "Upgrade", "HTTP/1.1");

	zasprintf(&msg, "<h1>%s</h1>", status_get(c->code));
	header_set(c, "Content-Length", "%d", strlen(msg));
	header_send(c);

	HTTPD_WRITE(c->fd, msg, strlen(msg));
}

static void
send_uri(struct Client *c)
{
	char *uri, *ptr;
	char path[PATH_MAX];
	char root[PATH_MAX];
	struct stat st;
	ssize_t n;
	char buf[BUFSIZ];
	struct magic_set *magic;

	if (c->uri[0] == '/')
		ZSTRDUP(uri, c->uri);
	else /* http:// */ {
		if ((uri = strchr(c->uri + 7, '/')))
			ZSTRDUP(uri, uri);
		else
			ZSTRDUP(uri, "/");
	}

	if ((ptr = strchr(uri, '?')))
		*ptr = '\0';

	if (!realpath(uri+1, path) ||
			!realpath("."/* TODO CONF ROOT */, root) ||
			strncmp(path, root, strlen(root)))
	{
		c->code = 404;
		return send_error(c);
	}

	if (stat(path, &st) == -1 || !S_ISREG(st.st_mode)
			|| (c->f = open(path, O_RDONLY)) == -1)
	{
		c->code = (errno == EACCES) ? 403 : 404;
		return send_error(c);
	}

	if ((magic = magic_open(MAGIC_MIME_TYPE)))
		magic_load(magic, NULL);

	c->code = 200;
	header_set(c, "Content-Length", "%llu", st.st_size);
	header_set(c, "Content-Type", "%s", (magic) ? magic_file(magic, path) : "text/plain");
	header_send(c);

	if (c->method != HEAD)
		while ((n = read(c->f, buf, BUFSIZ)) != -1)
		{
			if (n == 0)
				break;
			HTTPD_WRITE(c->fd, buf, n);
		}

	if (magic)
		magic_close(magic);
}

static char *
header_get(struct Client *c, const char *key)
{
	struct http_hdrs *h;
	SLIST_FOREACH(h, &c->reqh, next)
		if (!strcmp(key, h->key))
			return h->val;
	return NULL;
}

static void
header_set(struct Client *c, const char *key, const char *fmt, ...)
{
	struct http_hdrs *h;
	char *ptr = NULL;
	va_list args;

	va_start(args, fmt);
	vasprintf(&ptr, fmt, args);
	va_end(args);

	if (!ptr)
		return;

	mstack_push(ptr);

	SLIST_FOREACH(h, &c->resh, next)
	{
		if (!strcmp(h->key, key))
		{
			h->val = ptr;
			return;
		}
	}


	ZMALLOC(h, sizeof(*h));
	ZSTRDUP(h->key, key);
	h->val = ptr;
	SLIST_INSERT_HEAD(&c->resh, h, next);
}

static void
header_send(struct Client *c)
{
	struct st_code *st;
	char date[35];
	struct http_hdrs *h;

	for (st = status_code; st->code != 0; st++)
		if (st->code == c->code)
			break;

	/* if any */
	if (st->code == 0)
	{
		c->code = 500;
		return header_send(c);
	}

	header_set(c, "Connection", "close");
	header_set(c, "Date", getdate(date));
	header_set(c, "Server", "OpenHTTPD/1.0");

	zwrite(c->fd, "HTTP/1.1 %d %s\r\n", c->code, st->msg);
	SLIST_FOREACH(h, &c->resh, next)
		zwrite(c->fd, "%s: %s\r\n", h->key, h->val);
	zwrite(c->fd, "\r\n");
}

static char *
status_get(int code)
{
	struct st_code *st;

	for (st = status_code; st->code != 0; st++)
		if (st->code == code)
			return st->msg;
	return NULL;
}
