#ifndef H_REQUEST
#define H_REQUEST

#include <unistd.h>
#include "client.h"
void request_manage(struct Client *);

#define HTTPD_WRITE(fd, data, len)					\
	do {											\
		if (write(fd, data, len) == -1)				\
			client_destroy();						\
	} while (0)


#endif /* H_REQUEST */
