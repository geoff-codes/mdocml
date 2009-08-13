/*	$Id: man_term.c,v 1.18 2009/08/10 10:09:51 kristaps Exp $ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"
#include "man.h"

#define	INDENT		  7
#define	HALFINDENT	  3

#ifdef __linux__
extern	size_t		  strlcpy(char *, const char *, size_t);
extern	size_t		  strlcat(char *, const char *, size_t);
#endif

#define	MANT_LITERAL	 (1 << 0)

#define	DECL_ARGS 	  struct termp *p, \
			  int *fl, \
			  const struct man_node *n, \
			  const struct man_meta *m

struct	termact {
	int		(*pre)(DECL_ARGS);
	void		(*post)(DECL_ARGS);
};

static	int		  pre_B(DECL_ARGS);
static	int		  pre_BI(DECL_ARGS);
static	int		  pre_BR(DECL_ARGS);
static	int		  pre_HP(DECL_ARGS);
static	int		  pre_I(DECL_ARGS);
static	int		  pre_IB(DECL_ARGS);
static	int		  pre_IP(DECL_ARGS);
static	int		  pre_IR(DECL_ARGS);
static	int		  pre_PP(DECL_ARGS);
static	int		  pre_RB(DECL_ARGS);
static	int		  pre_RI(DECL_ARGS);
static	int		  pre_SH(DECL_ARGS);
static	int		  pre_SS(DECL_ARGS);
static	int		  pre_TP(DECL_ARGS);
static	int		  pre_br(DECL_ARGS);
static	int		  pre_fi(DECL_ARGS);
static	int		  pre_nf(DECL_ARGS);
static	int		  pre_r(DECL_ARGS);
static	int		  pre_sp(DECL_ARGS);

static	void		  post_B(DECL_ARGS);
static	void		  post_I(DECL_ARGS);
static	void		  post_SH(DECL_ARGS);
static	void		  post_SS(DECL_ARGS);
static	void		  post_i(DECL_ARGS);

static const struct termact termacts[MAN_MAX] = {
	{ pre_br, NULL }, /* br */
	{ NULL, NULL }, /* TH */
	{ pre_SH, post_SH }, /* SH */
	{ pre_SS, post_SS }, /* SS */
	{ pre_TP, NULL }, /* TP */
	{ pre_PP, NULL }, /* LP */
	{ pre_PP, NULL }, /* PP */
	{ pre_PP, NULL }, /* P */
	{ pre_IP, NULL }, /* IP */
	{ pre_HP, NULL }, /* HP */ 
	{ NULL, NULL }, /* SM */
	{ pre_B, post_B }, /* SB */
	{ pre_BI, NULL }, /* BI */
	{ pre_IB, NULL }, /* IB */
	{ pre_BR, NULL }, /* BR */
	{ pre_RB, NULL }, /* RB */
	{ NULL, NULL }, /* R */
	{ pre_B, post_B }, /* B */
	{ pre_I, post_I }, /* I */
	{ pre_IR, NULL }, /* IR */
	{ pre_RI, NULL }, /* RI */
	{ NULL, NULL }, /* na */ /* TODO: document that has no effect */
	{ pre_I, post_i }, /* i */
	{ pre_sp, NULL }, /* sp */
	{ pre_nf, NULL }, /* nf */
	{ pre_fi, NULL }, /* fi */
	{ pre_r, NULL }, /* r */
};

static	void		  print_head(struct termp *, 
				const struct man_meta *);
static	void		  print_body(DECL_ARGS);
static	void		  print_node(DECL_ARGS);
static	void		  print_foot(struct termp *, 
				const struct man_meta *);
static	void		  fmt_block_vspace(struct termp *, 
				const struct man_node *);
static	int		  arg_width(const struct man_node *);


int
man_run(struct termp *p, const struct man *m)
{
	int		 fl;

	print_head(p, man_meta(m));
	p->flags |= TERMP_NOSPACE;
	assert(man_node(m));
	assert(MAN_ROOT == man_node(m)->type);

	fl = 0;
	if (man_node(m)->child)
		print_body(p, &fl, man_node(m)->child, man_meta(m));
	print_foot(p, man_meta(m));

	return(1);
}


static void
fmt_block_vspace(struct termp *p, const struct man_node *n)
{
	term_newln(p);

	if (NULL == n->prev)
		return;

	if (MAN_SS == n->prev->tok)
		return;
	if (MAN_SH == n->prev->tok)
		return;

	term_vspace(p);
}


static int
arg_width(const struct man_node *n)
{
	int		 i, len;
	const char	*p;

	assert(MAN_TEXT == n->type);
	assert(n->string);

	p = n->string;

	if (0 == (len = (int)strlen(p)))
		return(-1);

	for (i = 0; i < len; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1)  {
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p));
	} else if (i == len)
		return(atoi(p));

	return(-1);
}


/* ARGSUSED */
static int
pre_I(DECL_ARGS)
{

	p->flags |= TERMP_UNDER;
	return(1);
}


/* ARGSUSED */
static int
pre_r(DECL_ARGS)
{

	p->flags &= ~TERMP_UNDER;
	p->flags &= ~TERMP_BOLD;
	return(1);
}


/* ARGSUSED */
static void
post_i(DECL_ARGS)
{

	if (n->nchild)
		p->flags &= ~TERMP_UNDER;
}


/* ARGSUSED */
static void
post_I(DECL_ARGS)
{

	p->flags &= ~TERMP_UNDER;
}


/* ARGSUSED */
static int
pre_fi(DECL_ARGS)
{

	*fl &= ~MANT_LITERAL;
	return(1);
}


/* ARGSUSED */
static int
pre_nf(DECL_ARGS)
{

	term_newln(p);
	*fl |= MANT_LITERAL;
	return(1);
}


/* ARGSUSED */
static int
pre_IR(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		if ( ! (i % 2))
			p->flags |= TERMP_UNDER;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		if ( ! (i % 2))
			p->flags &= ~TERMP_UNDER;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_IB(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		p->flags |= i % 2 ? TERMP_BOLD : TERMP_UNDER;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		p->flags &= i % 2 ? ~TERMP_BOLD : ~TERMP_UNDER;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_RB(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		if (i % 2)
			p->flags |= TERMP_BOLD;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		if (i % 2)
			p->flags &= ~TERMP_BOLD;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_RI(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		if ( ! (i % 2))
			p->flags |= TERMP_UNDER;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		if ( ! (i % 2))
			p->flags &= ~TERMP_UNDER;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_BR(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		if ( ! (i % 2))
			p->flags |= TERMP_BOLD;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		if ( ! (i % 2))
			p->flags &= ~TERMP_BOLD;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_BI(DECL_ARGS)
{
	const struct man_node *nn;
	int		 i;

	for (i = 0, nn = n->child; nn; nn = nn->next, i++) {
		p->flags |= i % 2 ? TERMP_UNDER : TERMP_BOLD;
		if (i > 0)
			p->flags |= TERMP_NOSPACE;
		print_node(p, fl, nn, m);
		p->flags &= i % 2 ? ~TERMP_UNDER : ~TERMP_BOLD;
	}
	return(0);
}


/* ARGSUSED */
static int
pre_B(DECL_ARGS)
{

	p->flags |= TERMP_BOLD;
	return(1);
}


/* ARGSUSED */
static void
post_B(DECL_ARGS)
{

	p->flags &= ~TERMP_BOLD;
}


/* ARGSUSED */
static int
pre_sp(DECL_ARGS)
{
	int		 i, len;

	if (NULL == n->child) {
		term_vspace(p);
		return(0);
	}

	len = atoi(n->child->string);
	if (0 == len)
		term_newln(p);
	for (i = 0; i < len; i++)
		term_vspace(p);

	return(0);
}


/* ARGSUSED */
static int
pre_br(DECL_ARGS)
{

	term_newln(p);
	return(0);
}


/* ARGSUSED */
static int
pre_HP(DECL_ARGS)
{

	/* TODO */
	return(1);
}


/* ARGSUSED */
static int
pre_PP(DECL_ARGS)
{

	switch (n->type) {
	case (MAN_BLOCK):
		fmt_block_vspace(p, n);
		break;
	default:
		p->offset = INDENT;
		break;
	}

	return(1);
}


/* ARGSUSED */
static int
pre_IP(DECL_ARGS)
{
	/* TODO */
#if 0
	const struct man_node *nn;
	size_t		 offs, sv;
	int		 ival;

	fmt_block_vspace(p, n);

	p->flags |= TERMP_NOSPACE;

	sv = p->offset;
	p->offset = INDENT;

	if (NULL == n->child)
		return(1);

	p->flags |= TERMP_NOBREAK;

	offs = sv;

	/*
	 * If the last token is number-looking (3m, 3n, 3) then
	 * interpret it as the width specifier, else we stick with the
	 * prior saved offset.  XXX - obviously not documented.
	 */
	for (nn = n->child; nn; nn = nn->next) {
		if (NULL == nn->next) {
			ival = arg_width(nn);
			if (ival >= 0) {
				offs = (size_t)ival;
				break;
			}
		}
		print_node(p, fl, nn, m);
	}

	p->rmargin = p->offset + offs;

	term_flushln(p);

	p->offset = offs;
	p->rmargin = p->maxrmargin;

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;

	return(0);
#endif
	return(1);
}


/* ARGSUSED */
static int
pre_TP(DECL_ARGS)
{
	/* TODO */
#if 0
	const struct man_node *nn;
	size_t		 offs;

	term_vspace(p);

	p->offset = INDENT;

	if (NULL == (nn = n->child))
		return(1);

	if (nn->line == n->line) {
		if (MAN_TEXT != nn->type)
			errx(1, "expected text line argument");
		offs = (size_t)atoi(nn->string);
		nn = nn->next;
	} else
		offs = INDENT;

	for ( ; nn; nn = nn->next)
		print_node(p, fl, nn, m);

	term_flushln(p);
	p->flags |= TERMP_NOSPACE;
	p->offset += offs;
	return(0);
#endif
	return(1);
}


/* ARGSUSED */
static int
pre_SS(DECL_ARGS)
{

	switch (n->type) {
	case (MAN_BLOCK):
		term_newln(p);
		if (n->prev)
			term_vspace(p);
		break;
	case (MAN_HEAD):
		p->flags |= TERMP_BOLD;
		p->offset = HALFINDENT;
		break;
	default:
		p->offset = INDENT;
		break;
	}

	return(1);
}


/* ARGSUSED */
static void
post_SS(DECL_ARGS)
{
	
	switch (n->type) {
	case (MAN_HEAD):
		term_newln(p);
		p->flags &= ~TERMP_BOLD;
		break;
	default:
		break;
	}
}


/* ARGSUSED */
static int
pre_SH(DECL_ARGS)
{
	/* 
	 * XXX: undocumented: using two `SH' macros in sequence has no
	 * vspace between calls, only a newline.
	 */
	switch (n->type) {
	case (MAN_BLOCK):
		if (n->prev && MAN_SH == n->prev->tok)
			if (NULL == n->prev->body->child)
				break;
		term_vspace(p);
		break;
	case (MAN_HEAD):
		p->flags |= TERMP_BOLD;
		p->offset = 0;
		break;
	case (MAN_BODY):
		p->offset = INDENT;
		break;
	default:
		break;
	}

	return(1);
}


/* ARGSUSED */
static void
post_SH(DECL_ARGS)
{
	
	switch (n->type) {
	case (MAN_HEAD):
		term_newln(p);
		p->flags &= ~TERMP_BOLD;
		break;
	case (MAN_BODY):
		term_newln(p);
		break;
	default:
		break;
	}
}


static void
print_node(DECL_ARGS)
{
	int		 c, sz;

	c = 1;

	switch (n->type) {
	case(MAN_TEXT):
		if (0 == *n->string) {
			term_vspace(p);
			break;
		}
		/*
		 * Note!  This is hacky.  Here, we recognise the `\c'
		 * escape embedded in so many -man pages.  It's supposed
		 * to remove the subsequent space, so we mark NOSPACE if
		 * it's encountered in the string.
		 */
		sz = (int)strlen(n->string);
		term_word(p, n->string);
		if (sz >= 2 && n->string[sz - 1] == 'c' &&
				n->string[sz - 2] == '\\')
			p->flags |= TERMP_NOSPACE;
		/* FIXME: this means that macro lines are munged!  */
		if (MANT_LITERAL & *fl) {
			p->flags |= TERMP_NOSPACE;
			term_flushln(p);
		}
		break;
	default:
		if (termacts[n->tok].pre)
			c = (*termacts[n->tok].pre)(p, fl, n, m);
		break;
	}

	if (c && n->child)
		print_body(p, fl, n->child, m);

	if (MAN_TEXT != n->type)
		if (termacts[n->tok].post)
			(*termacts[n->tok].post)(p, fl, n, m);
}


static void
print_body(DECL_ARGS)
{

	print_node(p, fl, n, m);
	if ( ! n->next)
		return;
	print_body(p, fl, n->next, m);
}


static void
print_foot(struct termp *p, const struct man_meta *meta)
{
	struct tm	*tm;
	char		*buf;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");

	tm = localtime(&meta->date);

	if (0 == strftime(buf, p->rmargin, "%B %d, %Y", tm))
		err(1, "strftime");

	term_vspace(p);

	p->flags |= TERMP_NOSPACE | TERMP_NOBREAK;
	p->rmargin = p->maxrmargin - strlen(buf);
	p->offset = 0;

	if (meta->source)
		term_word(p, meta->source);
	if (meta->source)
		term_word(p, "");
	term_flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;

	term_word(p, buf);
	term_flushln(p);

	free(buf);
}


static void
print_head(struct termp *p, const struct man_meta *meta)
{
	char		*buf, *title;

	p->rmargin = p->maxrmargin;
	p->offset = 0;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
		err(1, "malloc");

	if (meta->vol)
		(void)strlcpy(buf, meta->vol, p->rmargin);
	else
		*buf = 0;

	(void)snprintf(title, p->rmargin, "%s(%d)", 
			meta->title, meta->msec);

	p->offset = 0;
	p->rmargin = (p->maxrmargin - strlen(buf) + 1) / 2;
	p->flags |= TERMP_NOBREAK | TERMP_NOSPACE;

	term_word(p, title);
	term_flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin - strlen(title);

	term_word(p, buf);
	term_flushln(p);

	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;
	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;

	term_word(p, title);
	term_flushln(p);

	p->rmargin = p->maxrmargin;
	p->offset = 0;
	p->flags &= ~TERMP_NOSPACE;

	free(title);
	free(buf);
}

