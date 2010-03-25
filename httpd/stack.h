#ifndef H_STACK
#define H_STACK

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <err.h>

typedef struct Stack {
	void *adr;
	SLIST_ENTRY(Stack) next;
} Stack;

/* usefull macros */
#define XMALLOC(ptr, size)					\
	do {									\
		if (!(ptr = malloc(size)))			\
			err(EXIT_FAILURE, "malloc");	\
	} while (0)

#define XCALLOC(elm, n, size)				\
	do {									\
		if (!(elm = calloc(n, size)))		\
			err(EXIT_FAILURE, "calloc");	\
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

#define ZMALLOC(c, ptr, size)				\
	do {									\
		XMALLOC(ptr, size);					\
		mstack_push(c, ptr);				\
	} while (0)

#define ZCALLOC(c, elm, n, size)			\
	do {									\
		XCALLOC(elm, n, size);				\
		mstack_push(c, elm);				\
	} while (0)

#define ZSTRDUP(c, dst, src)				\
	do {									\
		XSTRDUP(dst, src);					\
		mstack_push(c, dst);				\
	} while (0)



#endif /* H_STACK */
