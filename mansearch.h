/*	$Id: manpath.h,v 1.5 2011/12/13 20:56:46 kristaps Exp $ */
/*
 * Copyright (c) 2012 Kristaps Dzonsons <kristaps@bsd.lv>
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
#ifndef MANSEARCH_H
#define MANSEARCH_H

struct	manpage {
	char		 file[MAXPATHLEN]; /* prefixed by manpath */
	char		*desc; /* description of manpage */
	int		 form; /* 0 == catpage */
};

__BEGIN_DECLS

int	mansearch(const struct manpaths *paths, /* manpaths */
		const char *arch, /* architecture */
		const char *sec,  /* manual section */
		int argc, /* size of argv */
		char *argv[],  /* search terms */
		struct manpage **res, /* results */
		size_t *ressz); /* results returned */

__END_DECLS

#endif /*!MANSEARCH_H*/
