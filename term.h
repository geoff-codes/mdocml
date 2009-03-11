/* $Id: term.h,v 1.19 2009/03/04 14:41:40 kristaps Exp $ */
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
#ifndef TERM_H
#define TERM_H

#include "mdoc.h"

#define	INDENT		  6

__BEGIN_DECLS

enum	tsym {
	TERMSYM_RBRACK = 	0,
	TERMSYM_LBRACK = 	1,
	TERMSYM_LARROW = 	2,
	TERMSYM_RARROW = 	3,
	TERMSYM_UARROW = 	4,
	TERMSYM_DARROW = 	5,
	TERMSYM_LSQUOTE = 	6,
	TERMSYM_RSQUOTE = 	7,
	TERMSYM_SQUOTE = 	8,
	TERMSYM_LDQUOTE = 	9,
	TERMSYM_RDQUOTE = 	10,
	TERMSYM_DQUOTE = 	11,
	TERMSYM_LT = 		12,
	TERMSYM_GT = 		13,
	TERMSYM_LE = 		14,
	TERMSYM_GE = 		15,
	TERMSYM_EQ = 		16,
	TERMSYM_NEQ = 		17,
	TERMSYM_ACUTE = 	18,
	TERMSYM_GRAVE = 	19,
	TERMSYM_PI = 		20,
	TERMSYM_PLUSMINUS = 	21,
	TERMSYM_INF = 		22,
	TERMSYM_INF2 = 		23,
	TERMSYM_NAN = 		24,
	TERMSYM_BAR = 		25,
	TERMSYM_BULLET = 	26,
	TERMSYM_AMP = 		27,
	TERMSYM_EM = 		28,
	TERMSYM_EN = 		29,
	TERMSYM_COPY = 		30,
	TERMSYM_ASTERISK =	31,
	TERMSYM_SLASH =		32,
	TERMSYM_HYPHEN =	33,
	TERMSYM_SPACE =		34,
	TERMSYM_PERIOD =	35,
	TERMSYM_BREAK =		36,
	TERMSYM_LANGLE =	37,
	TERMSYM_RANGLE =	38,
	TERMSYM_LBRACE =	39,
	TERMSYM_RBRACE =	40
};


enum	tstyle {
	TERMSTYLE_CLEAR	=	0,
	TERMSTYLE_BOLD =	1,
	TERMSTYLE_UNDER =	2,
	TERMSTYLE_MAX =		3
};

struct	termsym {
	const char	 *sym;
	size_t		  sz;
};

struct	termp {
	size_t		  rmargin;
	size_t		  maxrmargin;
	size_t		  maxcols;
	size_t		  offset;
	size_t		  col;
	int		  flags;
#define	TERMP_BOLD	 (1 << 0)	/* Embolden words. */
#define	TERMP_UNDERLINE	 (1 << 1)	/* Underline words. */
#define	TERMP_NOSPACE	 (1 << 2)	/* No space before words. */
#define	TERMP_NOLPAD	 (1 << 3)	/* No leftpad before flush. */
#define	TERMP_NOBREAK	 (1 << 4)	/* No break after flush. */
#define	TERMP_LITERAL	 (1 << 5)	/* Literal words. */
#define	TERMP_IGNDELIM	 (1 << 6)	/* Delims like regulars. */
#define	TERMP_NONOSPACE	 (1 << 7)	/* No space (no autounset). */
#define	TERMP_NONOBREAK	 (1 << 8)
	char		 *buf;
	struct termsym	 *symtab;	/* Special-symbol table. */
	struct termsym	 *styletab;	/* Style table. */
};

struct	termpair {
	struct termpair	 *ppair;
	int		  type;
#define	TERMPAIR_FLAG	 (1 << 0)
	int	  	  flag;
	size_t	  	  offset;
	size_t	  	  rmargin;
	int		  count;
};

#define	TERMPAIR_SETFLAG(termp, p, fl) \
	do { \
		assert(! (TERMPAIR_FLAG & (p)->type)); \
		(termp)->flags |= (fl); \
		(p)->flag = (fl); \
		(p)->type |= TERMPAIR_FLAG; \
	} while (0)

struct	termact {
	int		(*pre)(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
	void		(*post)(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
};

void			  newln(struct termp *);
void			  vspace(struct termp *);
void			  word(struct termp *, const char *);
void			  flushln(struct termp *);
void			  transcode(struct termp *, 
				const char *, size_t);
void			  subtree(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);


const	struct termact 	 *termacts;

__END_DECLS

#endif /*!TERM_H*/
