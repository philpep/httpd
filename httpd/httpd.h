#ifndef H_HTTPD
#define H_HTTPD

#include <sys/queue.h>

struct listener {
	pthread_t				tid;
	int 					fd;
	struct sockaddr_storage ss;
	in_port_t				port;
	int						running;
	TAILQ_ENTRY(listener)	entry;
};

struct httpd {
	TAILQ_HEAD(, listener) list;
	char *root;
};

extern struct httpd conf;

int parse_config(const char *);


#endif /* H_HTTPD */
