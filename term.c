/* $Id: term.c,v 1.4 2009/02/21 15:34:46 kristaps Exp $ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <ctype.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "term.h"
#include "private.h" /* XXX */

enum	termstyle {
	STYLE_CLEAR,
	STYLE_BOLD,
	STYLE_UNDERLINE
};

static	void		  termprint_r(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	void		  termprint_header(struct termp *,
				const struct mdoc_meta *);
static	void		  termprint_footer(struct termp *,
				const struct mdoc_meta *);

static	void		  pword(struct termp *, const char *, size_t);
static	void		  chara(struct termp *, char);
static	void		  escape(struct termp *, enum termstyle);


void
flushln(struct termp *p)
{
	size_t		 i, j, vsz, vis, maxvis;

	/*
	 * First, establish the maximum columns of "visible" content.
	 * This is usually the difference between the right-margin and
	 * an indentation, but can be, for tagged lists or columns, a
	 * small set of values.
	 */

	assert(p->offset < p->rmargin);
	maxvis = p->rmargin - p->offset;
	vis = 0;

	/*
	 * If in the standard case (left-justified), then begin with our
	 * indentation, otherwise (columns, etc.) just start spitting
	 * out text.
	 */

	if ( ! (p->flags & TERMP_NOLPAD))
		/* LINTED */
		for (j = 0; j < p->offset; j++)
			putchar(' ');

	for (i = 0; i < p->col; i++) {
		/*
		 * Count up visible word characters.  Control sequences
		 * (starting with the CSI) aren't counted. 
		 */
		assert( ! isspace(p->buf[i]));

		/* LINTED */
		for (j = i, vsz = 0; j < p->col; j++) {
			if (isspace(p->buf[j]))
				break;
			else if (27 == p->buf[j]) {
				assert(j + 4 <= p->col);
				j += 3;
			} else
				vsz++;
		}
		assert(vsz > 0);

		/*
		 * If a word is too long and we're within a line, put it
		 * on the next line.  Puke if we're being asked to write
		 * something that will exceed the right margin (i.e.,
		 * from a fresh line or when we're not allowed to break
		 * the line with TERMP_NOBREAK).
		 */

		if (vis && vis + vsz >= maxvis) {
			/* FIXME */
			if (p->flags & TERMP_NOBREAK)
				errx(1, "word breaks right margin");
			putchar('\n');
			for (j = 0; j < p->offset; j++)
				putchar(' ');
			vis = 0;
		} else if (vis + vsz >= maxvis) {
			/* FIXME */
			errx(1, "word breaks right margin");
		}

		/* 
		 * Write out the word and a trailing space.  Omit the
		 * space if we're the last word in the line.
		 */

		for ( ; i < p->col; i++) {
			if (isspace(p->buf[i]))
				break;
			putchar(p->buf[i]);
		}
		vis += vsz;
		if (i < p->col) {
			putchar(' ');
			vis++;
		}
	}

	/*
	 * If we're not to right-marginalise it (newline), then instead
	 * pad to the right margin and stay off.
	 */

	if (p->flags & TERMP_NOBREAK) {
		for ( ; vis <= maxvis; vis++)
			putchar(' ');
	} else
		putchar('\n');

	p->col = 0;
}


void
newln(struct termp *p)
{

	/* 
	 * A newline only breaks an existing line; it won't assert
	 * vertical space.
	 */
	p->flags |= TERMP_NOSPACE;
	if (0 == p->col) 
		return;
	flushln(p);
}


void
vspace(struct termp *p)
{

	/*
	 * Asserts a vertical space (a full, empty line-break between
	 * lines).
	 */
	newln(p);
	putchar('\n');
}


static void
chara(struct termp *p, char c)
{

	/* TODO: dynamically expand the buffer. */
	if (p->col + 1 >= p->maxcols)
		errx(1, "line overrun");
	p->buf[(p->col)++] = c;
}


static void
escape(struct termp *p, enum termstyle esc)
{

	if (p->col + 4 >= p->maxcols)
		errx(1, "line overrun");

	p->buf[(p->col)++] = 27;
	p->buf[(p->col)++] = '[';
	switch (esc) {
	case (STYLE_CLEAR):
		p->buf[(p->col)++] = '0';
		break;
	case (STYLE_BOLD):
		p->buf[(p->col)++] = '1';
		break;
	case (STYLE_UNDERLINE):
		p->buf[(p->col)++] = '4';
		break;
	default:
		abort();
		/* NOTREACHED */
	}
	p->buf[(p->col)++] = 'm';
}


static void
pword(struct termp *p, const char *word, size_t len)
{
	size_t		 i;

	assert(len > 0);

	if ( ! (p->flags & TERMP_NOSPACE))
		chara(p, ' ');

	p->flags &= ~TERMP_NOSPACE;

	if (p->flags & TERMP_BOLD)
		escape(p, STYLE_BOLD);
	if (p->flags & TERMP_UNDERLINE)
		escape(p, STYLE_UNDERLINE);

	/* TODO: escape patterns. */

	for (i = 0; i < len; i++) 
		chara(p, word[i]);

	if (p->flags & TERMP_BOLD ||
			p->flags & TERMP_UNDERLINE)
		escape(p, STYLE_CLEAR);
}


void
word(struct termp *p, const char *word)
{
	size_t 		 i, j, len;

	if (mdoc_isdelim(word))
		p->flags |= TERMP_NOSPACE;

	len = strlen(word);
	assert(len > 0);

	/* LINTED */
	for (j = i = 0; i < len; i++) {
		if ( ! isspace(word[i])) {
			j++;
			continue;
		}
		if (0 == j)
			continue;
		assert(i >= j);
		pword(p, &word[i - j], j);
		j = 0;
	}
	if (j > 0) {
		assert(i >= j);
		pword(p, &word[i - j], j);
	}
}


static void
termprint_r(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{
	int		 dochild;

	/* Pre-processing. */

	dochild = 1;

	if (MDOC_TEXT != node->type) {
		if (termacts[node->tok].pre)
			if ( ! (*termacts[node->tok].pre)(p, meta, node))
				dochild = 0;
	} else /* MDOC_TEXT == node->type */
		word(p, node->data.text.string);

	/* Children. */

	if (dochild && node->child)
		termprint_r(p, meta, node->child);

	/* Post-processing. */

	if (MDOC_TEXT != node->type) {
		if (termacts[node->tok].post)
			if ( ! (*termacts[node->tok].post)(p, meta, node))
				return;
	} 

	/* Siblings. */

	if (node->next)
		termprint_r(p, meta, node->next);
}


static void
termprint_footer(struct termp *p, const struct mdoc_meta *meta)
{
	struct tm	*tm;
	char		*buf, *os;
	size_t		 sz, osz, ssz, i;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (os = malloc(p->rmargin)))
		err(1, "malloc");

	tm = localtime(&meta->date);
	if (NULL == strftime(buf, p->rmargin, "%B %d, %Y", tm))
		err(1, "strftime");

	osz = strlcpy(os, meta->os, p->rmargin);

	sz = strlen(buf);
	ssz = sz + osz + 1;

	if (ssz > p->rmargin) {
		ssz -= p->rmargin;
		assert(ssz <= osz);
		os[osz - ssz] = 0;
		ssz = 1;
	} else
		ssz = p->rmargin - ssz + 1;

	printf("\n");
	printf("%s", os);
	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s\n", buf);
	fflush(stdout);

	free(buf);
	free(os);
}


static void
termprint_header(struct termp *p, const struct mdoc_meta *meta)
{
	char		*msec, *buf, *title, *pp;
	size_t		 ssz, tsz, ttsz, i;;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
		err(1, "malloc");

	if (NULL == (pp = mdoc_vol2a(meta->vol)))
		switch (meta->msec) {
		case (MSEC_1):
			/* FALLTHROUGH */
		case (MSEC_6):
			/* FALLTHROUGH */
		case (MSEC_7):
			pp = mdoc_vol2a(VOL_URM);
			break;
		case (MSEC_8):
			pp = mdoc_vol2a(VOL_SMM);
			break;
		case (MSEC_2):
			/* FALLTHROUGH */
		case (MSEC_3):
			/* FALLTHROUGH */
		case (MSEC_4):
			/* FALLTHROUGH */
		case (MSEC_5):
			pp = mdoc_vol2a(VOL_PRM);
			break;
		case (MSEC_9):
			pp = mdoc_vol2a(VOL_KM);
			break;
		default:
			/* FIXME: capitalise. */
			if (NULL == (pp = mdoc_msec2a(meta->msec)))
				pp = mdoc_msec2a(MSEC_local);
			break;
		}
	assert(pp);

	tsz = strlcpy(buf, pp, p->rmargin);
	assert(tsz < p->rmargin);

	if ((pp = mdoc_arch2a(meta->arch))) {
		tsz = strlcat(buf, " (", p->rmargin);
		assert(tsz < p->rmargin);
		tsz = strlcat(buf, pp, p->rmargin);
		assert(tsz < p->rmargin);
		tsz = strlcat(buf, ")", p->rmargin);
		assert(tsz < p->rmargin);
	}

	ttsz = strlcpy(title, meta->title, p->rmargin);

	if (NULL == (msec = mdoc_msec2a(meta->msec)))
		msec = "";

	ssz = (2 * (ttsz + 2 + strlen(msec))) + tsz + 2;

	if (ssz > p->rmargin) {
		if ((ssz -= p->rmargin) % 2)
			ssz++;
		ssz /= 2;
	
		assert(ssz <= ttsz);
		title[ttsz - ssz] = 0;
		ssz = 1;
	} else
		ssz = ((p->rmargin - ssz) / 2) + 1;

	printf("%s(%s)", title, msec);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s", buf);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s(%s)\n", title, msec);
	fflush(stdout);

	free(title);
	free(buf);
}


int
termprint(const struct mdoc_node *node,
		const struct mdoc_meta *meta)
{
	struct termp	 p;

	p.maxrmargin = 80; /* XXX */
	p.rmargin = p.maxrmargin;
	p.maxcols = 1024;
	p.offset = p.col = 0;
	p.flags = TERMP_NOSPACE;

	if (NULL == (p.buf = malloc(p.maxcols)))
		err(1, "malloc");

	termprint_header(&p, meta);
	termprint_r(&p, meta, node);
	termprint_footer(&p, meta);

	free(p.buf);

	return(1);
}


