#ifndef H_CLIENT
#define H_CLIENT

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <pthread.h>
#include <netinet/in.h>
#include <err.h>

#include "tools.h"

typedef struct Stack {
	void *adr;
	SLIST_ENTRY(Stack) next;
} Stack;

struct st_code {
	int code;
	char *msg;
};

struct http_hdrs {
	char *key;
	char *val;
	SLIST_ENTRY(http_hdrs) next;
};

struct Client {
	pthread_t			tid;
	int					fd;
	struct				sockaddr_in sin;
	SLIST_HEAD(, Stack) mstack;
	enum { HTTP11, HTTP10 } version;	/* HTTP version */
	enum { GET, HEAD, POST, OPTIONS,
		PUT, DELETE, TRACE, CONNECT, NONE } method;
	char				*uri;
	void				*body;		/* body data */
	size_t				bsize;		/* body size */
	SLIST_HEAD(, http_hdrs) reqh;	/* request headers */
	int					f;			/* open file */
	int					code;		/* status code */
	SLIST_HEAD(, http_hdrs) resh;		/* response headers */
	SLIST_ENTRY(Client) next;
};


SLIST_HEAD(, Client) clients;

struct Client *client_new(void);
struct Client *client_get(void);
void client_destroy(void);

void mstack_push(void *);

#define CLIENT_ADD(c)	SLIST_INSERT_HEAD(&clients, c, next)

/* usefull macros */
#define XMALLOC(ptr, size)					\
	do {									\
		if (!(ptr = malloc(size)))			\
			err(EXIT_FAILURE, "malloc");	\
	} while (0)

#define XSTRDUP(dst, src)					\
	do {									\
		if (!(dst = strdup(src)))			\
			err(EXIT_FAILURE, "strdup");	\
	} while (0)

#define XREALLOC(ptr, size)					\
	do {									\
		if (!(ptr = realloc(ptr, size)))	\
			err(EXIT_FAILURE, "realloc");   \
	} while (0)

#define ZMALLOC(ptr, size)					\
	do {									\
		XMALLOC(ptr, size);					\
		mstack_push(ptr);					\
	} while (0)

#define ZSTRDUP(dst, src)					\
	do {									\
		XSTRDUP(dst, src);					\
		mstack_push(dst);					\
	} while (0)


#endif /* H_CLIENT */
