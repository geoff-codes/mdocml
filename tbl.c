/*	$Id: tbl.c,v 1.12 2011/01/01 13:37:40 kristaps Exp $ */
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "roff.h"
#include "libmandoc.h"
#include "libroff.h"

enum rofferr
tbl_read(struct tbl *tbl, int ln, const char *p, int offs)
{
	int		 len;
	const char	*cp;

	cp = &p[offs];
	len = (int)strlen(cp);

	/*
	 * If we're in the options section and we don't have a
	 * terminating semicolon, assume we've moved directly into the
	 * layout section.  No need to report a warning: this is,
	 * apparently, standard behaviour.
	 */

	if (TBL_PART_OPTS == tbl->part && len)
		if (';' != cp[len - 1])
			tbl->part = TBL_PART_LAYOUT;

	/* Now process each logical section of the table.  */

	switch (tbl->part) {
	case (TBL_PART_OPTS):
		return(tbl_option(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	case (TBL_PART_LAYOUT):
		return(tbl_layout(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	case (TBL_PART_DATA):
		break;
	}

	/*
	 * This only returns zero if the line is empty, so we ignore it
	 * and continue on.
	 */
	return(tbl_data(tbl, ln, p) ? ROFF_TBL : ROFF_IGN);
}

struct tbl *
tbl_alloc(int pos, int line, void *data, const mandocmsg msg)
{
	struct tbl	*p;

	p = mandoc_calloc(1, sizeof(struct tbl));
	p->line = line;
	p->pos = pos;
	p->data = data;
	p->msg = msg;
	p->part = TBL_PART_OPTS;
	p->tab = '\t';
	p->linesize = 12;
	p->decimal = '.';
	return(p);
}

void
tbl_free(struct tbl *p)
{
	struct tbl_row	*rp;
	struct tbl_cell	*cp;
	struct tbl_span	*sp;
	struct tbl_dat	*dp;

	while (p->first_row) {
		rp = p->first_row;
		p->first_row = rp->next;
		while (rp->first) {
			cp = rp->first;
			rp->first = cp->next;
			free(cp);
		}
		free(rp);
	}

	while (p->first_span) {
		sp = p->first_span;
		p->first_span = sp->next;
		while (sp->first) {
			dp = sp->first;
			sp->first = dp->next;
			if (dp->string)
				free(dp->string);
			free(dp);
		}
		free(sp);
	}

	free(p);
}

void
tbl_restart(struct tbl *tbl)
{

	tbl->part = TBL_PART_LAYOUT;
}

const struct tbl_span *
tbl_span(const struct tbl *tbl)
{

	assert(tbl);
	return(tbl->last_span);
}

void
tbl_end(struct tbl *tbl)
{

	if (NULL == tbl->first_span || NULL == tbl->first_span->first)
		TBL_MSG(tbl, MANDOCERR_TBLNODATA, tbl->line, tbl->pos);
}
