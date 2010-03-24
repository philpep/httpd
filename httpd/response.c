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
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "httpd.h"
#include "response.h"
#include "version.h"

#define INTERNAL_SERVER_ERROR "HTTP/1.1 500 Internal Server Error\r\n" \
	"Connection: close\r\n" \
	"Server: "SERVER_STRING"\r\n\r\n"

static void send_error(struct Client *c);
static void send_uri(struct Client *c);
static void header_send(struct Client *c);
static void header_set(struct Client *c, const char *key, const char *fmt, ...);
static char *header_get(struct Client *c, const char *key);
static char *status_get(int code);

static struct st_code {
	int code;
	char *msg;
} status_code[] = {
#include "status_code.h"
};

void
request_manage(struct Client *c)
{
	char buf[BUFSIZ];
	void *data = NULL;
	ssize_t n = 0;
	size_t nread = 0;
	off_t offset = 0;
	char **hdrs, **line;
	size_t hdrs_size, i;
	struct http_hdrs *hel;	/* header element */
	char *conn;

	if (c->bsize != 0) {
		XMALLOC(data, c->bsize);
		memcpy(data, c->body, c->bsize);
		nread = c->bsize;
	}

	for(;;)
	{
		if ((c->body = memmem(data+offset, nread-offset, "\r\n\r\n", 4))) {
			*(char*)c->body = '\0';
			c->body += 4;
			c->bsize = data + nread - c->body;
			break;
		}

		n = read(c->fd, buf, BUFSIZ);

		if (n == -1 || n == 0)
			break;

		nread += n;

		XREALLOC(data, nread);
		memcpy(data+nread-n, buf, n);

		offset = (nread-n-4 > 0) ? nread-n-4 : 0;

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
	
		c->smethod = line[0];

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
	
		c->sversion = line[2];

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
			if (!(line[0] = strstr(hdrs[i], ": "))) {
				c->code = 400;
				break;
			}

			*line[0] = '\0';
			line[0] += 2;
			ZMALLOC(hel, sizeof(*hel));
			hel->key = hdrs[i];
			hel->val = line[0];
			SLIST_INSERT_HEAD(&c->reqh, hel, next);
			/*printf("'%s'->'%s'\n", hel->key, hel->val);*/
		}
	} while (0);

	if ((conn = header_get(c, "Connection")) &&
			!strcmp(conn, "close")) {
		c->conn = CLOSE;
	}
	else
		c->conn = KEEP_ALIVE;

	if (c->code != 0)
		send_error(c);
	else
		send_uri(c);

	if (c->conn == KEEP_ALIVE)
		return request_manage(c);

	client_destroy();
}

static void
send_error(struct Client *c)
{
	char *msg;
	if (c->code == 101 || c->code == 505)
		header_set(c, "Upgrade", "HTTP/1.1");

	zasprintf(&msg, "<h1 style=\"text-align: center;\">%d - %s</h1>",
			c->code, status_get(c->code));
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
	char *requested = NULL;
	struct vhost *vh; 
	struct stat st;
	ssize_t n;
	char buf[BUFSIZ];

	if (c->uri[0] == '/') {
		ZSTRDUP(uri, c->uri);
		c->vhost = header_get(c, "Host");
	}
	else /* http:// */ {
		if ((uri = strchr(c->uri + 7, '/')))
		{
			ZCALLOC(c->vhost, strlen(c->uri+7), sizeof(char));
			memcpy(c->vhost, c->uri+7, uri - c->uri-7);
			ZSTRDUP(uri, uri);
		}
		else {
			c->vhost = c->uri + 7;
			ZSTRDUP(uri, "/");
		}
	}

	if (c->vhost == NULL) {
		c->code = 400;
		return send_error(c);
	}

	/* Sometimes port is in vhost request */
	if ((ptr = strchr(c->vhost, ':')))
		*ptr = '\0';

	if ((ptr = strchr(uri, '?'))) {
		c->query_string = ptr+1;
		*ptr = '\0';
	}

	c->path_info = uri;

	/* search vhost */
	TAILQ_FOREACH(vh, &conf.vhosts, entry) {
		if (!strcmp(vh->host, c->vhost)) {
			ZMALLOC(requested, sizeof(char) *
					(strlen(vh->root)+strlen(uri)));
			strcpy(requested, vh->root);
			strcat(requested, uri);
			break;
		}
	}

	if (!requested) {
		c->code = 404;
		return send_error(c);
	}

	/* Check if requested is in root directory */
	if (!realpath(requested, path) ||
			!realpath(vh->root, root) ||
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

	c->code = 200;
	header_set(c, "Content-Length", "%llu", st.st_size);
	header_set(c, "Content-Type", "%s", get_mime_type(path));
	header_send(c);

	if (c->method != HEAD)
		while ((n = read(c->f, buf, BUFSIZ)) != -1)
		{
			if (n == 0)
				break;
			HTTPD_WRITE(c->fd, buf, n);
		}

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

	if (c->conn == CLOSE)
		header_set(c, "Connection", "close");
	header_set(c, "Date", get_date(date));
	header_set(c, "Server", SERVER_STRING);

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
