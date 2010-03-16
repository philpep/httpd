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

#include <unistd.h>
#include "client.h"

struct Client *
client_new(void)
{
	struct Client *c;

	if ((c = calloc(1, sizeof(*c))) == NULL)
		err(1, "calloc");

	c->f = -1;

	return c;
}

void
client_destroy(void)
{
	struct Client *c = client_get();
	Stack *el;

	close(c->fd);

	if (c->f != -1)
		close(c->f);

	while(!SLIST_EMPTY(&c->mstack))
	{
		el = SLIST_FIRST(&c->mstack);
		SLIST_REMOVE_HEAD(&c->mstack, next);
		free(el->adr);
		free(el);
	}

	SLIST_REMOVE(&clients, c, Client, next);

	pthread_exit(NULL);
}

struct Client *
client_get(void)
{
	pthread_t tid;
	struct Client *c;

	tid = pthread_self();

	SLIST_FOREACH(c, &clients, next)
	{
		if (c->tid == tid)
			break;
	}
	return c;
}

void
mstack_push(void *ptr)
{
	struct Client *c = NULL;
	Stack *el;

	c = client_get();

	if (c && ptr) {
		XMALLOC(el, sizeof(*el));
		el->adr = ptr;
		SLIST_INSERT_HEAD(&c->mstack, el, next);
	}
}
