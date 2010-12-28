/*	$Id: libroff.h,v 1.1 2010/12/28 10:51:03 kristaps Exp $ */
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
#ifndef LIBROFF_H
#define LIBROFF_H

__BEGIN_DECLS

enum	tbl_tok {
	TBL_TOK_OPENPAREN = 0,
	TBL_TOK_CLOSEPAREN,
	TBL_TOK_COMMA,
	TBL_TOK_SEMICOLON,
	TBL_TOK_PERIOD,
	TBL_TOK_SPACE,
	TBL_TOK_TAB,
	TBL_TOK_NIL,
	TBL_TOK__MAX
};

enum	tbl_part {
	TBL_PART_OPTS, /* in options (first line) */
	TBL_PART_LAYOUT, /* describing layout */
	TBL_PART_DATA  /* creating data rows */
};

struct	tbl {
	enum tbl_part	  part;
	char		  buf[BUFSIZ];
	char		  tab; /* cell-separator */
	char		  decimal; /* decimal point */
	int		  linesize;
	char		  delims[2];
	int		  opts;
#define	TBL_OPT_CENTRE	 (1 << 0)
#define	TBL_OPT_EXPAND	 (1 << 1)
#define	TBL_OPT_BOX	 (1 << 2)
#define	TBL_OPT_DBOX	 (1 << 3)
#define	TBL_OPT_ALLBOX	 (1 << 4)
#define	TBL_OPT_NOKEEP	 (1 << 5)
#define	TBL_OPT_NOSPACE	 (1 << 6)
};

struct tbl	*tbl_alloc(void);
void		 tbl_free(struct tbl *);
void		 tbl_reset(struct tbl *);
enum rofferr 	 tbl_read(struct tbl *, int, const char *, int);
enum tbl_tok	 tbl_next(struct tbl *, const char *, int *);

__END_DECLS

#endif /*LIBROFF_H*/
