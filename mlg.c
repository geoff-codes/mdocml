/* $Id: mlg.c,v 1.5 2008/12/04 16:19:52 kristaps Exp $ */
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
#include <ctype.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"
#include "ml.h"

/* TODO: literal tokens. */

#define	COLUMNS		  72
#define	INDENT		  4
#define	MAXINDENT	  10

enum	md_tok {
	MD_TEXT,
	MD_INLINE_IN,
	MD_INLINE_OUT,
	MD_BLK_IN,
	MD_BLK_OUT,
};

struct	md_mlg {
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;

	struct md_mbuf	 *mbuf;
	struct rofftree	 *tree;
	size_t		  indent;
	size_t		  pos;
	enum md_tok	  last;
	void		 *arg;
	ml_begintag	  begintag;
	ml_endtag	  endtag;
	ml_begin	  begin;
	ml_end		  end;
	int		  flags;
#define	ML_OVERRIDE_ONE	 (1 << 0)
#define	ML_OVERRIDE_ALL	 (1 << 1)
};


static	void		 mlg_roffmsg(void *arg, enum roffmsg, 
				const char *, const char *, char *);
static	int		 mlg_roffhead(void *, const struct tm *, 
				const char *, const char *, 
				const char *, const char *);
static	int		 mlg_rofftail(void *);
static	int		 mlg_roffin(void *, int, int *, char **);
static	int		 mlg_roffdata(void *, int, 
				const char *, char *);
static	int		 mlg_rofftoken(void *, int, int);
static	int		 mlg_roffout(void *, int);
static	int		 mlg_roffblkin(void *, int, int *, char **);
static	int		 mlg_roffblkout(void *, int);
static	int		 mlg_roffspecial(void *, int, int *, 
				char **, char **);
static	int		 mlg_roffblkheadin(void *, int, 
				int *, char **);
static	int		 mlg_roffblkheadout(void *, int);
static	int		 mlg_roffblkbodyin(void *, int, 
				int *, char **);
static	int		 mlg_roffblkbodyout(void *, int);

static	int		 mlg_beginblk(struct md_mlg *, enum md_ns, int, 
				int *, char **);
static	int		 mlg_endblk(struct md_mlg *, enum md_ns, int);
static	int		 mlg_begintag(struct md_mlg *, enum md_ns, 
				int, int *, char **);
static	int		 mlg_endtag(struct md_mlg *, enum md_ns, int);
static	int	  	 mlg_indent(struct md_mlg *);
static	int		 mlg_newline(struct md_mlg *);
static	void		 mlg_mode(struct md_mlg *, enum md_tok);
static	int		 mlg_data(struct md_mlg *, int, 
				const char *, char *);
static	void		 mlg_err(struct md_mlg *, const char *, 
				const char *, char *);
static	void		 mlg_warn(struct md_mlg *, const char *, 
				const char *, char *);
static	void		 mlg_msg(struct md_mlg *, enum roffmsg, 
				const char *, const char *, char *);

#ifdef __linux__
extern	size_t		 strlcat(char *, const char *, size_t);
extern	size_t		 strlcpy(char *, const char *, size_t);
#endif


static int
mlg_beginblk(struct md_mlg *p, enum md_ns ns, int tok, 
		int *argc, char **argv)
{
	if (0 != p->pos) {
		if ( ! mlg_newline(p))
			return(0);
		if ( ! mlg_indent(p))
			return(0);
	} else if ( ! mlg_indent(p))
		return(0);

	p->indent++;
	mlg_mode(p, MD_BLK_IN);

	if ( ! mlg_begintag(p, ns, tok, argc, argv))
		return(0);
	return(mlg_newline(p));
}


static int
mlg_endblk(struct md_mlg *p, enum md_ns ns, int tok)
{

	p->indent--;

	if (0 != p->pos) {
		if ( ! mlg_newline(p))
			return(0);
		if ( ! mlg_indent(p))
			return(0);
	} else if ( ! mlg_indent(p))
		return(0);

	mlg_mode(p, MD_BLK_OUT);
	if ( ! mlg_endtag(p, ns, tok))
		return(0);
	return(mlg_newline(p));
}


static int
mlg_begintag(struct md_mlg *p, enum md_ns ns, int tok,
		int *argc, char **argv)
{
	ssize_t		 res;

	/* TODO: extra rules for block/inline. */

	if ( ! ml_nputs(p->mbuf, "<", 1, &p->pos))
		return(0);

	res = (*p->begintag)(p->mbuf, p->args, ns, tok,
			argc, (const char **)argv);
	if (-1 == res)
		return(0);

	assert(res >= 0);
	p->pos += (size_t)res;

	/* TODO: extra rules for block/inline. */

	return(ml_nputs(p->mbuf, ">", 1, &p->pos));
}


static int
mlg_endtag(struct md_mlg *p, enum md_ns ns, int tok)
{
	ssize_t		 res;

	/* TODO: extra rules for block/inline. */

	if ( ! ml_nputs(p->mbuf, "</", 2, &p->pos))
		return(0);

	res = (*p->endtag)(p->mbuf, p->args, ns, tok);
	if (-1 == res)
		return(0);

	assert(res >= 0);
	p->pos += (size_t)res;

	/* TODO: extra rules for block/inline. */

	return(ml_nputs(p->mbuf, ">", 1, &p->pos));
}


static int
mlg_indent(struct md_mlg *p)
{
	size_t		 count;

	count = p->indent > MAXINDENT ? (size_t)MAXINDENT : p->indent;
	count *= INDENT;

	assert(0 == p->pos);
	return(ml_putchars(p->mbuf, ' ', count, &p->pos));
}


static int
mlg_newline(struct md_mlg *p)
{
	size_t		 dummy;

	if ( ! ml_nputs(p->mbuf, "\n", 1, &dummy))
		return(0);
	p->pos = 0;
	return(1);
}


static void
mlg_mode(struct md_mlg *p, enum md_tok ns)
{

	p->flags &= ~ML_OVERRIDE_ONE;
	p->last = ns;
}


static int
mlg_data(struct md_mlg *p, int space, const char *start, char *buf)
{
	size_t		 sz;
	char		*bufp;
	int		 c;

	assert(p->mbuf);
	assert(0 != p->indent);

	if (ML_OVERRIDE_ONE & p->flags || 
			ML_OVERRIDE_ALL & p->flags)
		space = 0;

	while (*buf) {
		while (*buf && isspace(*buf))
			buf++;

		if (0 == *buf)
			break;

		bufp = buf;
		while (*buf && ! isspace(*buf))
			buf++;

		if (0 != *buf)
			*buf++ = 0;

		sz = strlen(bufp);

		if (0 == p->pos) {
			if ( ! mlg_indent(p))
				return(0);

			c = ml_nputstring(p->mbuf, bufp, sz, &p->pos);
			if (0 == c) {
				mlg_err(p, start, bufp, "invalid "
						"character sequence");
				return(0);
			} else if (c > 1) {
				mlg_warn(p, start, bufp, "bogus "
						"character sequence");
				return(0);
			} else if (-1 == c)
				return(0);

			if (p->indent * INDENT + sz >= COLUMNS)
				if ( ! mlg_newline(p))
					return(0);
			if ( ! (ML_OVERRIDE_ALL & p->flags))
				space = 1;
			continue;
		}

		if (space && sz + p->pos >= COLUMNS) {
			if ( ! mlg_newline(p))
				return(0);
			if ( ! mlg_indent(p))
				return(0);
		} else if (space) {
			if ( ! ml_nputs(p->mbuf, " ", 1, &p->pos))
				return(0);
		}

		c = ml_nputstring(p->mbuf, bufp, sz, &p->pos);
		if (0 == c) {
			mlg_err(p, start, bufp, "invalid "
					"character sequence");
			return(0);
		} else if (c > 1) {
			mlg_warn(p, start, bufp, "bogus "
					"character sequence");
			return(0);
		} else if (-1 == c)
			return(0);

		if ( ! (ML_OVERRIDE_ALL & p->flags))
			space = 1;
	}

	return(1);
}


int
mlg_line(struct md_mlg *p, char *buf)
{

	return(roff_engine(p->tree, buf));
}


int
mlg_exit(struct md_mlg *p, int flush)
{
	int		 c;

	c = roff_free(p->tree, flush);
	free(p);
	return(c);
}


struct md_mlg *
mlg_alloc(const struct md_args *args, 
		const struct md_rbuf *rbuf,
		struct md_mbuf *mbuf, 
		ml_begintag begintag, ml_endtag endtag,
		ml_begin begin, ml_end end)
{
	struct roffcb	 cb;
	struct md_mlg	*p;

	cb.roffhead = mlg_roffhead;
	cb.rofftail = mlg_rofftail;
	cb.roffin = mlg_roffin;
	cb.roffout = mlg_roffout;
	cb.roffblkin = mlg_roffblkin;
	cb.roffblkheadin = mlg_roffblkheadin;
	cb.roffblkheadout = mlg_roffblkheadout;
	cb.roffblkbodyin = mlg_roffblkbodyin;
	cb.roffblkbodyout = mlg_roffblkbodyout;
	cb.roffblkout = mlg_roffblkout;
	cb.roffspecial = mlg_roffspecial;
	cb.roffmsg = mlg_roffmsg;
	cb.roffdata = mlg_roffdata;
	cb.rofftoken = mlg_rofftoken;

	if (NULL == (p = calloc(1, sizeof(struct md_mlg))))
		err(1, "calloc");

	p->args = args;
	p->mbuf = mbuf;
	p->rbuf = rbuf;
	p->begintag = begintag;
	p->endtag = endtag;
	p->begin = begin;
	p->end = end;

	if (NULL == (p->tree = roff_alloc(&cb, p))) {
		free(p);
		return(NULL);
	}

	return(p);
}


static int
mlg_roffhead(void *arg, const struct tm *tm, const char *os, 
		const char *title, const char *sec, const char *vol)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	mlg_mode(p, MD_BLK_IN);
	if ( ! (*p->begin)(p->mbuf, p->args, tm, os, title, sec, vol))
		return(0);

	p->indent++;
	return(mlg_newline(p));
}


static int
mlg_rofftail(void *arg)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	if (0 != p->pos && ! mlg_newline(p))
		return(0);

	mlg_mode(p, MD_BLK_OUT);
	if ( ! (*p->end)(p->mbuf, p->args))
		return(0);

	return(mlg_newline(p));
}


/* ARGSUSED */
static int
mlg_roffspecial(void *arg, int tok, int *argc, char **argv, char **more)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	switch (tok) {
	case (ROFF_Ns):
		p->flags |= ML_OVERRIDE_ONE;
		break;
	case (ROFF_Sm):
		assert(*more);
		if (0 == strcmp(*more, "on"))
			p->flags |= ML_OVERRIDE_ALL;
		else
			p->flags &= ~ML_OVERRIDE_ALL;
		break;
	default:
		break;
	}

	return(1);
}


static int
mlg_roffblkin(void *arg, int tok, int *argc, char **argv)
{

	return(mlg_beginblk((struct md_mlg *)arg, 
				MD_NS_BLOCK, tok, argc, argv));
}


static int
mlg_roffblkout(void *arg, int tok)
{

	return(mlg_endblk((struct md_mlg *)arg, MD_NS_BLOCK, tok));
}


static int
mlg_roffblkbodyin(void *arg, int tok, int *argc, char **argv)
{

	return(mlg_beginblk((struct md_mlg *)arg, 
				MD_NS_BODY, tok, argc, argv));
}


static int
mlg_roffblkbodyout(void *arg, int tok)
{

	return(mlg_endblk((struct md_mlg *)arg, MD_NS_BODY, tok));
}


static int
mlg_roffblkheadin(void *arg, int tok, int *argc, char **argv)
{

	return(mlg_beginblk((struct md_mlg *)arg, 
				MD_NS_HEAD, tok, argc, argv));
}


static int
mlg_roffblkheadout(void *arg, int tok)
{

	return(mlg_endblk((struct md_mlg *)arg, MD_NS_HEAD, tok));
}


static int
mlg_roffin(void *arg, int tok, int *argc, char **argv)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	/* FIXME: this part. */

	if ( ! (ML_OVERRIDE_ONE & p->flags) && 
			! (ML_OVERRIDE_ALL & p->flags) && 
			p->pos + 11 > COLUMNS) 
		if ( ! mlg_newline(p))
			return(0);

	if (0 != p->pos && (MD_TEXT == p->last || 
				MD_INLINE_OUT == p->last)
			&& ! (ML_OVERRIDE_ONE & p->flags)
			&& ! (ML_OVERRIDE_ALL & p->flags))
		if ( ! ml_nputs(p->mbuf, " ", 1, &p->pos))
			return(0);

	if (0 == p->pos && ! mlg_indent(p))
		return(0);

	mlg_mode(p, MD_INLINE_IN);
	return(mlg_begintag(p, MD_NS_INLINE, tok, argc, argv));
}


static int
mlg_roffout(void *arg, int tok)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	if (0 == p->pos && ! mlg_indent(p))
		return(0);

	mlg_mode(p, MD_INLINE_OUT);
	return(mlg_endtag(p, MD_NS_INLINE, tok));
}


static void
mlg_roffmsg(void *arg, enum roffmsg lvl, 
		const char *buf, const char *pos, char *msg)
{

	mlg_msg((struct md_mlg *)arg, lvl, buf, pos, msg);
}


static int
mlg_rofftoken(void *arg, int space, int value)
{
	struct md_mlg	*p;
	const char	*seq;
	size_t		 sz, res;

	assert(arg);
	p = (struct md_mlg *)arg;

	switch (value) {
	case (ROFFTok_Sp_A):
		seq = "\\a";
		sz = 2;
		break;
	case (ROFFTok_Sp_B):
		seq = "\\b";
		sz = 2;
		break;
	case (ROFFTok_Sp_F):
		seq = "\\f";
		sz = 2;
		break;
	case (ROFFTok_Sp_N):
		seq = "\\n";
		sz = 2;
		break;
	case (ROFFTok_Sp_R):
		seq = "\\r";
		sz = 2;
		break;
	case (ROFFTok_Sp_T):
		seq = "\\t";
		sz = 2;
		break;
	case (ROFFTok_Sp_V):
		seq = "\\v";
		sz = 2;
		break;
	case (ROFFTok_Space):
		seq = "&nbsp;";
		sz = 6;
		break;
	case (ROFFTok_Hyphen):
		seq = "&#8208;";
		sz = 7;
		break;
	case (ROFFTok_Em):
		seq = "&#8212;";
		sz = 7;
		break;
	case (ROFFTok_En):
		seq = "&#8211;";
		sz = 7;
		break;
	case (ROFFTok_Ge):
		seq = "&#8805;";
		sz = 7;
		break;
	case (ROFFTok_Le):
		seq = "&#8804;";
		sz = 7;
		break;
	case (ROFFTok_Rquote):
		seq = "&#8221;";
		sz = 7;
		break;
	case (ROFFTok_Lquote):
		seq = "&#8220;";
		sz = 7;
		break;
	case (ROFFTok_Uparrow):
		seq = "&#8593;";
		sz = 7;
		break;
	case (ROFFTok_Acute):
		seq = "&#180;";
		sz = 6;
		break;
	case (ROFFTok_Grave):
		seq = "&#96;";
		sz = 5;
		break;
	case (ROFFTok_Pi):
		seq = "&#960;";
		sz = 6;
		break;
	case (ROFFTok_Ne):
		seq = "&#8800;";
		sz = 7;
		break;
	case (ROFFTok_Lt):
		seq = "&lt;";
		sz = 4;
		break;
	case (ROFFTok_Gt):
		seq = "&gt;";
		sz = 4;
		break;
	case (ROFFTok_Plusmin):
		seq = "&#177;";
		sz = 6;
		break;
	case (ROFFTok_Infty):
		seq = "&#8734;";
		sz = 7;
		break;
	case (ROFFTok_Bar):
		seq = "&#124;";
		sz = 6;
		break;
	case (ROFFTok_Nan):
		seq = "Nan";
		sz = 3;
		break;
	case (ROFFTok_Quote):
		seq = "&quot;";
		sz = 6;
		break;
	default:
		/* TODO: print error. */
		return(0);
	}

	if (space && ! ml_nputs(p->mbuf, " ", 1, &res))
		return(0);
	p->pos += res;

	if ( ! ml_nputs(p->mbuf, seq, sz, &res))
		return(0);
	p->pos += res;

	return(1);
}


static int
mlg_roffdata(void *arg, int space, const char *start, char *buf)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	if ( ! mlg_data(p, space, start, buf))
		return(0);

	mlg_mode(p, MD_TEXT);
	return(1);
}


static void
mlg_err(struct md_mlg *p, const char *buf, const char *pos, char *msg)
{

	mlg_msg(p, ROFF_ERROR, buf, pos, msg);
}


static void
mlg_warn(struct md_mlg *p, const char *buf, const char *pos, char *msg)
{

	mlg_msg(p, ROFF_WARN, buf, pos, msg);
}


static void
mlg_msg(struct md_mlg *p, enum roffmsg lvl, 
		const char *buf, const char *pos, char *msg)
{
	char		*level;

	switch (lvl) {
	case (ROFF_WARN):
		if ( ! (MD_WARN_ALL & p->args->warnings))
			return;
		level = "warning";
		break;
	case (ROFF_ERROR):
		level = "error";
		break;
	default:
		abort();
	}
	
	if (pos)
		(void)fprintf(stderr, "%s:%zu: %s: %s (column %zu)\n", 
				p->rbuf->name, p->rbuf->line, level, 
				msg, pos - buf);
	else
		(void)fprintf(stderr, "%s: %s: %s\n", 
				p->rbuf->name, level, msg);

}
