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

