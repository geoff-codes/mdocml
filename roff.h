/*	$Id: mdoc.h,v 1.78 2010/05/13 06:22:11 kristaps Exp $ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@bsd.lv>
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
#ifndef ROFF_H
#define ROFF_H

enum	rofferr {
	ROFF_CONT,
	ROFF_IGN,
	ROFF_ERROR
};

__BEGIN_DECLS

struct	roff;

void	 	  roff_free(struct roff *);
struct	roff	 *roff_alloc(void *);
void		  roff_reset(struct roff *);
enum	rofferr	  roff_parseln(struct roff *, int, char **, size_t *);
int		  roff_endparse(struct roff *);

__END_DECLS

#endif /*!ROFF_H*/
