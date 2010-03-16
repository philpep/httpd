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
