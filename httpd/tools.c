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
#include <libgen.h>
#include "response.h"

static struct mime_type {
	const char *ext;
	const char *val;
} m_type[] = {
#include "mime_types.h"
};

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

char *
unsplit(char **str, char *sep)
{
	size_t i, n, len = 0;
	char *ret;

	for (n = 0; str[n]; n++)
		len += strlen(str[n]);

	if (sep)
		len += n*strlen(sep);


	ZMALLOC(ret, len * sizeof(char));
	ret[0] = '\0';

	for (i = 0; i < n; i++) {
		ret = strcat(ret, str[i]);
		if (sep && i != (n-1))
			ret = strcat(ret, sep);
	}

	return ret;
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
get_date(char *date)
{
	time_t ts;
	ts = time(NULL);
	strftime(date, sizeof(char)*35, "%a, %d %b %Y %X GMT", gmtime(&ts));
	return date;
}

const char *
get_mime_type(const char *path)
{
	size_t i;
	char *ext;

	if ((ext = basename(path)) &&
			(ext = strrchr(ext, '.'))) {
		ext++;
		for (i = 0; i < sizeof(m_type)/sizeof(*m_type); i++)
		{
			if (!strcmp(m_type[i].ext, ext))
				return m_type[i].val;
		}
	}

	return "text/plain; charset=utf-8";
}
