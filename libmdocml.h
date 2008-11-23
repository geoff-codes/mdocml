/* $Id: libmdocml.h,v 1.5 2008/11/23 22:30:53 kristaps Exp $ */
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

struct 	md_params_dummy {
	int		 dummy;
};

struct 	md_params_html4_strict {
	int		 dummy;
};

union	md_params {
	struct md_params_dummy dummy;
	struct md_params_html4_strict html4_strict;
};

enum	md_type {
	MD_DUMMY,		/* Dummy type echoes input. */
	MD_HTML4_STRICT		/* HTML4.01-strict. */
};

struct	md_args {
	union md_params	 params;/* Parameters for parser. */
	enum md_type	 type;	/* Type of parser. */
	int		 dbg;	/* Debug level. */
#define	MD_DBG_TREE	 1
};

struct	md_buf {
	int		 fd;	/* Open file descriptor. */
	char		*name;	/* Name of file/socket/whatever. */
	char		*buf;	/* Buffer for storing data. */
	size_t		 bufsz;	/* Size of buf. */
};

__BEGIN_DECLS

/* Run the parser over prepared input and output buffers.  Returns -1 on
 * failure and 0 on success.
 */
int	md_run(const struct md_args *,
		const struct md_buf *, const struct md_buf *);

__END_DECLS

#endif /*!LIBMDOCML_H*/
