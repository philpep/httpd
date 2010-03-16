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
#include <stdarg.h>
#include "response.h"

char **
splitstr(char *str, const char *sep, size_t *n)
{
	int i, size;
	char *p, *tmp, **split;

	if (!str || !sep || sep[0] == '\0') {
		warnx("Bad call to splitstr");
		return NULL;
	}

	for (i = size = 0; str[i]; i++)
		if (str[i] == *sep)
			size++;

	size += 2;
	ZMALLOC(split, size * sizeof(char*));

	i = 0;
	for (p = str; p;)
		while ((tmp = strsep(&p, sep)))
			if (*tmp) {
				while (*tmp == ' ' || *tmp == '\t')
					tmp++;
				ZSTRDUP(split[i], tmp);
				i++;
			}

	split[i] = NULL;

	if (n)
		*n = i;

	return split;
}

int
zasprintf(char **ptr, const char *fmt, ...)
{
	va_list args;
	int ret = 0;
	
	va_start(args, fmt);
	ret = vasprintf(ptr, fmt, args);
	va_end(args);

	if (*ptr)
		mstack_push(*ptr);

	return ret;
}

void
zwrite(int fd, const char *fmt, ...)
{
	char *ptr = NULL;
	va_list args;
	int n;

	va_start(args, fmt);
	n = vasprintf(&ptr, fmt, args);
	va_end(args);

	if (ptr && n > 0)
	{
		mstack_push(ptr);
		HTTPD_WRITE(fd, ptr, n);
	}
}

char *
getdate(char *date)
{
	time_t ts;
	ts = time(NULL);
	strftime(date, sizeof(char)*35, "%a, %d %b %Y %X GMT", gmtime(&ts));
	return date;
}

