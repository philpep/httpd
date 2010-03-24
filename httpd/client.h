#ifndef H_CLIENT
#define H_CLIENT

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <pthread.h>
#include <netinet/in.h>
#include <err.h>

#include "tools.h"
#include "stack.h"

struct http_hdrs {
	char *key;
	char *val;
	SLIST_ENTRY(http_hdrs) next;
};

struct Client {
	pthread_t			tid;
	int					fd;
	struct				sockaddr_storage ss;
	SLIST_HEAD(, Stack) mstack;
	enum { HTTP11, HTTP10 } version;	/* HTTP version */
	char				*sversion;	/* version string */
	enum { GET, HEAD, POST, OPTIONS,
		PUT, DELETE, TRACE, CONNECT, NONE } method;
	char				*smethod;	/* method string */
	char				*uri;
	char				*path_info;		/* PATH_INFO for CGI */
	char				*cgi;	/* path for CGI file */
	char				*query_string;	/* QUERY_STRING for CGI */
	void				*body;		/* body data */
	size_t				bsize;		/* body size */
	char				*vhost;		/* virtual host */
	SLIST_HEAD(, http_hdrs) reqh;	/* request headers */
	int					f;			/* open file */
	int					code;		/* status code */
	enum { KEEP_ALIVE, CLOSE } conn; /* connection type (keep-alive / close */
	off_t				offset; /* data ofset */
	SLIST_HEAD(, http_hdrs) resh;		/* response headers */
	SLIST_ENTRY(Client) next;
};


SLIST_HEAD(, Client) clients;

struct Client *client_new(void);
struct Client *client_get(void);
void client_destroy(void);

void mstack_push(void *);

#define CLIENT_ADD(c)	SLIST_INSERT_HEAD(&clients, c, next)

#endif /* H_CLIENT */
