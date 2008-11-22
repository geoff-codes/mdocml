/* $Id: libmdocml.h,v 1.1.1.1 2008/11/22 14:53:29 kristaps Exp $ */
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
#ifndef LIBMDOCML_H
#define LIBMDOCML_H

#include <sys/types.h>

struct	md_rbuf {
	int		 fd;
	const char	*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 line;
};

struct	md_mbuf {
	int		 fd;
	const char	*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 pos;
};

enum	md_type {
	MD_DUMMY
};

__BEGIN_DECLS

int	md_run(enum md_type, struct md_mbuf *, struct md_rbuf *);

__END_DECLS

#endif /*!LIBMDOCML_H*/
