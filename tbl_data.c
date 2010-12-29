/*	$Id: data.c,v 1.11 2009/09/12 16:05:34 kristaps Exp $ */
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

#include "mandoc.h"
#include "libmandoc.h"
#include "libroff.h"

static	void	data(struct tbl *, struct tbl_span *, 
			int, const char *, int *);

void
data(struct tbl *tbl, struct tbl_span *dp, 
		int ln, const char *p, int *pos)
{
	struct tbl_dat	*dat;
	int		 sv;

	/* FIXME: warn about losing data contents if cell is HORIZ. */

	dat = mandoc_calloc(1, sizeof(struct tbl_dat));

	if (dp->last) {
		dp->last->next = dat;
		dp->last = dat;
	} else
		dp->last = dp->first = dat;

	sv = *pos;
	while (p[*pos] && p[*pos] != tbl->tab)
		(*pos)++;

	dat->string = mandoc_malloc(*pos - sv + 1);
	memcpy(dat->string, &p[sv], *pos - sv);
	dat->string[*pos - sv] = '\0';

	if (p[*pos])
		(*pos)++;

	/* XXX: do the strcmps, then malloc(). */

	if ( ! strcmp(dat->string, "_"))
		dat->flags |= TBL_DATA_HORIZ;
	else if ( ! strcmp(dat->string, "="))
		dat->flags |= TBL_DATA_DHORIZ;
	else if ( ! strcmp(dat->string, "\\_"))
		dat->flags |= TBL_DATA_NHORIZ;
	else if ( ! strcmp(dat->string, "\\="))
		dat->flags |= TBL_DATA_NDHORIZ;
}

struct tbl_span *
tbl_data(struct tbl *tbl, int ln, const char *p)
{
	struct tbl_span	*dp;
	int		 pos;

	pos = 0;

	if ('\0' == p[pos]) {
		TBL_MSG(tbl, MANDOCERR_TBL, ln, pos);
		return(NULL);
	}

	dp = mandoc_calloc(1, sizeof(struct tbl_span));

	if ( ! strcmp(p, "_")) {
		dp->flags |= TBL_SPAN_HORIZ;
		return(dp);
	} else if ( ! strcmp(p, "=")) {
		dp->flags |= TBL_SPAN_DHORIZ;
		return(dp);
	}

	while ('\0' != p[pos])
		data(tbl, dp, ln, p, &pos);

	return(dp);
}
