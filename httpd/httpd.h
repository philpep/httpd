#ifndef H_HTTPD
#define H_HTTPD

#include <sys/queue.h>
#include <pthread.h>
#include <sys/socket.h>

struct listener {
	pthread_t				tid;
	int 					fd;
	struct sockaddr_storage ss;
	in_port_t				port;
	int						running;
	TAILQ_ENTRY(listener)	entry;
};

struct vhost {
	char	*root;
	char	*host;
	TAILQ_ENTRY(vhost)	entry;
};

struct httpd {
	TAILQ_HEAD(, listener) list;
	TAILQ_HEAD(, vhost) vhosts;
	char *root;
};

extern struct httpd conf;

int parse_config(const char *);


#endif /* H_HTTPD */
