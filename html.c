/* $Id: xml.c,v 1.10 2008/12/03 14:39:59 kristaps Exp $ */
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"
#include "ml.h"


static	ssize_t		html_endtag(struct md_mbuf *, 
				const struct md_args *, 
				enum md_ns, int);
static	ssize_t		html_begintag(struct md_mbuf *, 
				const struct md_args *, 
				enum md_ns, int, 
				const int *, const char **);
static	int		html_begin(struct md_mbuf *, 
				const struct md_args *);
static	int		html_end(struct md_mbuf *, 
				const struct md_args *);
static	ssize_t		html_blocktagname(struct md_mbuf *,
				const struct md_args *, int);
static	ssize_t		html_blocktagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **);
static	ssize_t		html_inlinetagname(struct md_mbuf *,
				const struct md_args *, int);
static	ssize_t		html_inlinetagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **);


static int 
html_begin(struct md_mbuf *mbuf, const struct md_args *args)
{
	size_t		 res;

	res = 0;
	if ( ! ml_puts(mbuf, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD "
				"HTML 4.01//EN\" \"http://www.w3.org"
				"/TR/html4/strict.dtd\">\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, "<html>\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, "<head>\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, " <title>Manual page</title>\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, " <meta http-equiv=\"Content-Type\" "
				"content=\"text/html; "
				"charset=utf-8\">\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, " <meta name=\"resource-type\" "
				"content=\"document\">\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, "</head>\n", &res))
		return(0);
	if ( ! ml_puts(mbuf, "<body>", &res))
		return(0);

	return(1);
}


static int 
html_end(struct md_mbuf *mbuf, const struct md_args *args)
{
	size_t		 res;

	res = 0;
	if ( ! ml_puts(mbuf, "</body>\n</html>", &res))
		return(0);

	return(1);
}


static ssize_t
html_blocktagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok)
{
	size_t		 res;

	res = 0;

	switch (tok) {
	case (ROFF_Sh):
		if ( ! ml_puts(mbuf, "blockquote", &res))
			return(-1);
		break;
	case (ROFF_Bd):
		if ( ! ml_puts(mbuf, "pre", &res))
			return(-1);
		break;
	case (ROFF_Bl):
		if ( ! ml_puts(mbuf, "ul", &res))
			return(-1);
		break;
	case (ROFF_It):
		if ( ! ml_puts(mbuf, "li", &res))
			return(-1);
		break;
	default:
		if ( ! ml_puts(mbuf, "div", &res))
			return(-1);
		break;
	}

	return((size_t)res);
}


/* ARGSUSED */
static ssize_t
html_blocktagargs(struct md_mbuf *mbuf, const struct md_args *args, 
		int tok, const int *argc, const char **argv)
{

	switch (tok) {
	default:
		return(0);
	}

	return(-1);
}


/* ARGSUSED */
static ssize_t
html_inlinetagargs(struct md_mbuf *mbuf, const struct md_args *args, 
		int tok, const int *argc, const char **argv)
{

	switch (tok) {
	default:
		return(0);
	}

	return(-1);
}


static ssize_t
html_inlinetagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok)
{
	size_t		 res;

	res = 0;

	switch (tok) {
	case (ROFF_Sh):
		if ( ! ml_puts(mbuf, "h1", &res))
			return(-1);
		break;
	case (ROFF_Ss):
		if ( ! ml_puts(mbuf, "h2", &res))
			return(-1);
		break;
	default:
		if ( ! ml_puts(mbuf, "span", &res))
			return(-1);
		break;
	}

	return((ssize_t)res);
}


static ssize_t 
html_begintag(struct md_mbuf *mbuf, const struct md_args *args, 
		enum md_ns ns, int tok, 
		const int *argc, const char **argv)
{

	assert(ns != MD_NS_DEFAULT);
	if (MD_NS_BLOCK == ns) {
		if ( ! html_blocktagname(mbuf, args, tok))
			return(0);
		return(html_blocktagargs(mbuf, args, 
					tok, argc, argv));
	}

	if ( ! html_inlinetagname(mbuf, args, tok))
		return(0);
	return(html_inlinetagargs(mbuf, args, tok, argc, argv));
}


static ssize_t 
html_endtag(struct md_mbuf *mbuf, const struct md_args *args, 
		enum md_ns ns, int tok)
{

	assert(ns != MD_NS_DEFAULT);
	if (MD_NS_BLOCK == ns)
		return(html_blocktagname(mbuf, args, tok));

	return(html_inlinetagname(mbuf, args, tok));
}


int
md_line_html(void *data, char *buf)
{

	return(mlg_line((struct md_mlg *)data, buf));
}


int
md_exit_html(void *data, int flush)
{

	return(mlg_exit((struct md_mlg *)data, flush));
}


void *
md_init_html(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{

	return(mlg_alloc(args, rbuf, mbuf, html_begintag, 
				html_endtag, html_begin, html_end));
}

