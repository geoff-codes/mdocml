/*	$Id: libmandoc.h,v 1.12 2011/03/17 08:49:34 kristaps Exp $ */
/*
 * Copyright (c) 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#ifndef LIBMANDOC_H
#define LIBMANDOC_H

__BEGIN_DECLS

void	 mandoc_msg(enum mandocerr, struct mparse *, 
		int, int, const char *);
void	 mandoc_vmsg(enum mandocerr, struct mparse *, 
		int, int, const char *, ...);
int	 mandoc_special(char *);
char	*mandoc_strdup(const char *);
char	*mandoc_getarg(struct mparse *, char **, int, int *);
char	*mandoc_normdate(struct mparse *, char *, int, int);
int	 mandoc_eos(const char *, size_t, int);
int	 mandoc_hyph(const char *, const char *);

__END_DECLS

#endif /*!LIBMANDOC_H*/
