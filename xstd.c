/* $Id: xstd.c,v 1.4 2009/01/20 13:44:05 kristaps Exp $ */
/*
 * Copyright (c) 2008 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"

/*
 * Contains wrappers for common functions to simplify their general
 * usage throughout this codebase.
 */

#ifdef __linux__
extern	size_t		 strlcat(char *, const char *, size_t);
extern	size_t		 strlcpy(char *, const char *, size_t);
#endif


int
xstrncmp(const char *p1, const char *p2, size_t sz)
{

	return(0 == strncmp(p1, p2, sz));
}

int
xstrcmp(const char *p1, const char *p2)
{

	return(0 == strcmp(p1, p2));
}

int
xstrlcat(char *dst, const char *src, size_t sz)
{

	return(strlcat(dst, src, sz) < sz);
}

int
xstrlcpy(char *dst, const char *src, size_t sz)
{

	return(strlcpy(dst, src, sz) < sz);
}

void *
xcalloc(size_t num, size_t sz)
{
	void		*p;

	if (NULL == (p = calloc(num, sz)))
		err(EXIT_FAILURE, "calloc");
	return(p);
}

char *
xstrdup(const char *p)
{
	char		*pp;

	if (NULL == (pp = strdup(p)))
		err(EXIT_FAILURE, "strdup");
	return(pp);
}

int
xstrlcats(char *buf, const struct mdoc_node *n, size_t sz)
{
	char		 *p;

	assert(sz > 0);
	assert(buf);
	*buf = 0;

	for ( ; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		p = n->data.text.string;
		if ( ! xstrlcat(buf, p, sz))
			return(0);
		if (n->next && ! xstrlcat(buf, " ", sz))
			return(0);
	}

	return(1);
}

#ifdef __linux__
/*	$OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
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

#include <sys/types.h>
#include <string.h>


size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}
#endif
