/*	$Id: tbl_data.c,v 1.9 2011/01/02 12:04:23 kristaps Exp $ */
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "libmandoc.h"
#include "libroff.h"

static	void	data(struct tbl_node *, struct tbl_span *, 
			int, const char *, int *);

void
data(struct tbl_node *tbl, struct tbl_span *dp, 
		int ln, const char *p, int *pos)
{
	struct tbl_dat	*dat;
	struct tbl_cell	*cp;
	int		 sv;

	cp = NULL;
	if (dp->last && dp->last->layout)
		cp = dp->last->layout->next;
	else if (NULL == dp->last)
		cp = dp->layout->first;

	/* Skip over spanners to data formats. */

	while (cp && (TBL_CELL_VERT == cp->pos || 
				TBL_CELL_DVERT == cp->pos))
		cp = cp->next;

	/* FIXME: warn about losing data contents if cell is HORIZ. */

	dat = mandoc_calloc(1, sizeof(struct tbl_dat));
	dat->layout = cp;

	if (NULL == dat->layout)
		TBL_MSG(tbl, MANDOCERR_TBLEXTRADAT, ln, *pos);

	if (dp->last) {
		dp->last->next = dat;
		dp->last = dat;
	} else
		dp->last = dp->first = dat;

	sv = *pos;
	while (p[*pos] && p[*pos] != tbl->opts.tab)
		(*pos)++;

	dat->string = mandoc_malloc(*pos - sv + 1);
	memcpy(dat->string, &p[sv], *pos - sv);
	dat->string[*pos - sv] = '\0';

	if (p[*pos])
		(*pos)++;

	if ( ! strcmp(dat->string, "_"))
		dat->pos = TBL_DATA_HORIZ;
	else if ( ! strcmp(dat->string, "="))
		dat->pos = TBL_DATA_DHORIZ;
	else if ( ! strcmp(dat->string, "\\_"))
		dat->pos = TBL_DATA_NHORIZ;
	else if ( ! strcmp(dat->string, "\\="))
		dat->pos = TBL_DATA_NDHORIZ;
	else
		dat->pos = TBL_DATA_DATA;

	if (TBL_CELL_HORIZ == dat->layout->pos ||
			TBL_CELL_DHORIZ == dat->layout->pos)
		if (TBL_DATA_DATA == dat->pos && '\0' != *dat->string)
			TBL_MSG(tbl, MANDOCERR_TBLIGNDATA, ln, sv);
}

int
tbl_data(struct tbl_node *tbl, int ln, const char *p)
{
	struct tbl_span	*dp;
	struct tbl_row	*rp;
	int		 pos;

	pos = 0;

	if ('\0' == p[pos]) {
		TBL_MSG(tbl, MANDOCERR_TBL, ln, pos);
		return(0);
	}

	/* 
	 * Choose a layout row: take the one following the last parsed
	 * span's.  If that doesn't exist, use the last parsed span's.
	 * If there's no last parsed span, use the first row.  This can
	 * be NULL!
	 */

	if (tbl->last_span) {
		assert(tbl->last_span->layout);
		rp = tbl->last_span->layout->next;
		if (NULL == rp)
			rp = tbl->last_span->layout;
	} else
		rp = tbl->first_row;

	dp = mandoc_calloc(1, sizeof(struct tbl_span));
	dp->tbl = &tbl->opts;
	dp->layout = rp;
	dp->head = tbl->first_head;

	if (tbl->last_span) {
		tbl->last_span->next = dp;
		tbl->last_span = dp;
	} else {
		tbl->last_span = tbl->first_span = dp;
		dp->flags |= TBL_SPAN_FIRST;
	}

	if ( ! strcmp(p, "_")) {
		dp->pos = TBL_SPAN_HORIZ;
		return(1);
	} else if ( ! strcmp(p, "=")) {
		dp->pos = TBL_SPAN_DHORIZ;
		return(1);
	}

	dp->pos = TBL_SPAN_DATA;

	while ('\0' != p[pos])
		data(tbl, dp, ln, p, &pos);

	return(1);
}
