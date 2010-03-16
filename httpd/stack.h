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

#define ZMALLOC(ptr, size)					\
	do {									\
		XMALLOC(ptr, size);					\
		mstack_push(ptr);					\
	} while (0)

#define ZCALLOC(elm, n, size)				\
	do {									\
		XCALLOC(elm, n, size);				\
		mstack_push(elm);					\
	} while (0)

#define ZSTRDUP(dst, src)					\
	do {									\
		XSTRDUP(dst, src);					\
		mstack_push(dst);					\
	} while (0)



#endif /* H_STACK */
