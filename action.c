/* $Id: action.c,v 1.44 2009/03/20 15:14:01 kristaps Exp $ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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
#include <sys/utsname.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"

/*
 * Actions are executed on macros after they've been post-validated: in
 * other words, a macro will not be "acted upon" until all of its
 * children have been filled in (post-fix order).
 */

enum	merr {
	ENOWIDTH
};

enum	mwarn {
	WBADSEC,
	WNOWIDTH,
	WBADDATE
};

#define	PRE_ARGS  struct mdoc *m, const struct mdoc_node *n
#define	POST_ARGS struct mdoc *m

struct	actions {
	int	(*pre)(PRE_ARGS);
	int	(*post)(POST_ARGS);
};

static	int	  pwarn(struct mdoc *, int, int, enum mwarn);
static	int	  perr(struct mdoc *, int, int, enum merr);

static	int	  post_ar(POST_ARGS);
static	int	  post_bl(POST_ARGS);
static	int	  post_bl_width(POST_ARGS);
static	int	  post_bl_tagwidth(POST_ARGS);
static	int	  post_bd(POST_ARGS);
static	int	  post_dd(POST_ARGS);
static	int	  post_dt(POST_ARGS);
static	int	  post_nm(POST_ARGS);
static	int	  post_os(POST_ARGS);
static	int	  post_prol(POST_ARGS);
static	int	  post_sh(POST_ARGS);
static	int	  post_std(POST_ARGS);

static	int	  pre_bd(PRE_ARGS);

#define	merr(m, t) perr((m), (m)->last->line, (m)->last->pos, (t))
#define	mwarn(m, t) pwarn((m), (m)->last->line, (m)->last->pos, (t))

const	struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, post_dd }, /* Dd */ 
	{ NULL, post_dt }, /* Dt */ 
	{ NULL, post_os }, /* Os */ 
	{ NULL, post_sh }, /* Sh */ 
	{ NULL, NULL }, /* Ss */ 
	{ NULL, NULL }, /* Pp */ 
	{ NULL, NULL }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ pre_bd, post_bd }, /* Bd */ 
	{ NULL, NULL }, /* Ed */
	{ NULL, post_bl }, /* Bl */ 
	{ NULL, NULL }, /* El */
	{ NULL, NULL }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ NULL, post_ar }, /* Ar */
	{ NULL, NULL }, /* Cd */
	{ NULL, NULL }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ NULL, post_std }, /* Ex */
	{ NULL, NULL }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ 
	{ NULL, NULL }, /* Fl */
	{ NULL, NULL }, /* Fn */ 
	{ NULL, NULL }, /* Ft */ 
	{ NULL, NULL }, /* Ic */ 
	{ NULL, NULL }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ NULL, NULL }, /* Nd */ 
	{ NULL, post_nm }, /* Nm */ 
	{ NULL, NULL }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ NULL, post_std }, /* Rv */
	{ NULL, NULL }, /* St */
	{ NULL, NULL }, /* Va */
	{ NULL, NULL }, /* Vt */ 
	{ NULL, NULL }, /* Xr */
	{ NULL, NULL }, /* %A */
	{ NULL, NULL }, /* %B */
	{ NULL, NULL }, /* %D */
	{ NULL, NULL }, /* %I */
	{ NULL, NULL }, /* %J */
	{ NULL, NULL }, /* %N */
	{ NULL, NULL }, /* %O */
	{ NULL, NULL }, /* %P */
	{ NULL, NULL }, /* %R */
	{ NULL, NULL }, /* %T */
	{ NULL, NULL }, /* %V */
	{ NULL, NULL }, /* Ac */
	{ NULL, NULL }, /* Ao */
	{ NULL, NULL }, /* Aq */
	{ NULL, NULL }, /* At */ 
	{ NULL, NULL }, /* Bc */
	{ NULL, NULL }, /* Bf */ 
	{ NULL, NULL }, /* Bo */
	{ NULL, NULL }, /* Bq */
	{ NULL, NULL }, /* Bsx */
	{ NULL, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, NULL }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ NULL, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ NULL, NULL }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */
	{ NULL, NULL }, /* Po */
	{ NULL, NULL }, /* Pq */
	{ NULL, NULL }, /* Qc */
	{ NULL, NULL }, /* Ql */
	{ NULL, NULL }, /* Qo */
	{ NULL, NULL }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ NULL, NULL }, /* So */
	{ NULL, NULL }, /* Sq */
	{ NULL, NULL }, /* Sm */
	{ NULL, NULL }, /* Sx */
	{ NULL, NULL }, /* Sy */
	{ NULL, NULL }, /* Tn */
	{ NULL, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ NULL, NULL }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ NULL, NULL }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ NULL, NULL }, /* Bt */
	{ NULL, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ NULL, NULL }, /* Ud */
	{ NULL, NULL }, /* Lb */
	{ NULL, NULL }, /* Ap */
	{ NULL, NULL }, /* Lp */
	{ NULL, NULL }, /* Lk */
	{ NULL, NULL }, /* Mt */
	{ NULL, NULL }, /* Brq */
	{ NULL, NULL }, /* Bro */
	{ NULL, NULL }, /* Brc */
	{ NULL, NULL }, /* %C */
	{ NULL, NULL }, /* Es */
	{ NULL, NULL }, /* En */
	{ NULL, NULL }, /* Dx */
};


int
mdoc_action_pre(struct mdoc *m, const struct mdoc_node *n)
{

	switch (n->type) {
	case (MDOC_ROOT):
		break;
	case (MDOC_TEXT):
		break;
	default:
		if (NULL == mdoc_actions[m->last->tok].pre)
			break;
		return((*mdoc_actions[m->last->tok].pre)(m, n));
	}
	return(1);
}


int
mdoc_action_post(struct mdoc *m)
{

	if (MDOC_ACTED & m->last->flags)
		return(1);
	m->last->flags |= MDOC_ACTED;

	switch (m->last->type) {
	case (MDOC_TEXT):
		break;
	case (MDOC_ROOT):
		break;
	default:
		if (NULL == mdoc_actions[m->last->tok].post)
			break;
		return((*mdoc_actions[m->last->tok].post)(m));
	}
	return(1);
}


static int
perr(struct mdoc *m, int line, int pos, enum merr type)
{
	char		*p;

	p = NULL;

	switch (type) {
	case (ENOWIDTH):
		p = "missing width argument";
		break;
	}

	assert(p);
	return(mdoc_perr(m, line, pos, p));
}


static int
pwarn(struct mdoc *m, int line, int pos, enum mwarn type)
{
	char		*p;
	int		 c;

	p = NULL;
	c = WARN_SYNTAX;

	switch (type) {
	case (WBADSEC):
		p = "inappropriate document section in manual section";
		c = WARN_COMPAT;
		break;
	case (WNOWIDTH):
		p = "cannot determine default width";
		break;
	case (WBADDATE):
		p = "malformed date syntax";
		break;
	}

	assert(p);
	return(mdoc_pwarn(m, line, pos, c, p));
}


static int
post_std(POST_ARGS)
{

	/*
	 * If '-std' is invoked without an argument, fill it in with our
	 * name (if it's been set).
	 */

	if (NULL == m->last->args)
		return(1);
	if (m->last->args->argv[0].sz)
		return(1);

	assert(m->meta.name);

	m->last->args->argv[0].value = xcalloc(1, sizeof(char *));
	m->last->args->argv[0].sz = 1;
	m->last->args->argv[0].value[0] = xstrdup(m->meta.name);
	return(1);
}


static int
post_nm(POST_ARGS)
{
	char		 buf[64];

	if (m->meta.name)
		return(1);

	(void)xstrlcpys(buf, m->last->child, sizeof(buf));
	m->meta.name = xstrdup(buf);

	return(1);
}


static int
post_sh(POST_ARGS)
{
	enum mdoc_sec	 sec;
	char		 buf[64];

	/*
	 * We keep track of the current section /and/ the "named"
	 * section, which is one of the conventional ones, in order to
	 * check ordering.
	 */

	if (MDOC_HEAD != m->last->type)
		return(1);

	(void)xstrlcpys(buf, m->last->child, sizeof(buf));
	if (SEC_CUSTOM != (sec = mdoc_atosec(buf)))
		m->lastnamed = sec;

	switch ((m->lastsec = sec)) {
	case (SEC_RETURN_VALUES):
		/* FALLTHROUGH */
	case (SEC_ERRORS):
		switch (m->meta.msec) {
		case (2):
			/* FALLTHROUGH */
		case (3):
			/* FALLTHROUGH */
		case (9):
			break;
		default:
			return(mwarn(m, WBADSEC));
		}
		break;
	default:
		break;
	}
	return(1);
}


static int
post_dt(POST_ARGS)
{
	struct mdoc_node *n;
	const char	 *cp;
	char		 *ep;
	long		  lval;

	if (m->meta.title)
		free(m->meta.title);
	if (m->meta.vol)
		free(m->meta.vol);
	if (m->meta.arch)
		free(m->meta.arch);

	m->meta.title = m->meta.vol = m->meta.arch = NULL;
	m->meta.msec = 0;

	/* Handles: `.Dt' 
	 *   --> title = unknown, volume = local, msec = 0, arch = NULL
	 */

	if (NULL == (n = m->last->child)) {
		m->meta.title = xstrdup("unknown");
		m->meta.vol = xstrdup("local");
		return(post_prol(m));
	}

	/* Handles: `.Dt TITLE' 
	 *   --> title = TITLE, volume = local, msec = 0, arch = NULL
	 */

	m->meta.title = xstrdup(n->string);

	if (NULL == (n = n->next)) {
		m->meta.vol = xstrdup("local");
		return(post_prol(m));
	}

	/* Handles: `.Dt TITLE SEC'
	 *   --> title = TITLE, volume = SEC is msec ? 
	 *           format(msec) : SEC,
	 *       msec = SEC is msec ? atoi(msec) : 0,
	 *       arch = NULL
	 */

	if ((cp = mdoc_a2msec(n->string))) {
		m->meta.vol = xstrdup(cp);
		errno = 0;
		lval = strtol(n->string, &ep, 10);
		if (n->string[0] != '\0' && *ep == '\0')
			m->meta.msec = (int)lval;
	} else 
		m->meta.vol = xstrdup(n->string);

	if (NULL == (n = n->next))
		return(post_prol(m));

	/* Handles: `.Dt TITLE SEC VOL'
	 *   --> title = TITLE, volume = VOL is vol ?
	 *       format(VOL) : 
	 *           VOL is arch ? format(arch) : 
	 *               VOL
	 */

	if ((cp = mdoc_a2vol(n->string))) {
		free(m->meta.vol);
		m->meta.vol = xstrdup(cp);
		n = n->next;
	} else {
		cp = mdoc_a2arch(n->string);
		if (NULL == cp) {
			free(m->meta.vol);
			m->meta.vol = xstrdup(n->string);
		} else
			m->meta.arch = xstrdup(cp);
	}	

	/* Ignore any subsequent parameters... */

	return(post_prol(m));
}


static int
post_os(POST_ARGS)
{
	char		  buf[64];
	struct utsname	  utsname;

	if (m->meta.os)
		free(m->meta.os);

	(void)xstrlcpys(buf, m->last->child, sizeof(buf));

	if (0 == buf[0]) {
		if (-1 == uname(&utsname))
			return(mdoc_err(m, "utsname"));
		(void)xstrlcpy(buf, utsname.sysname, sizeof(buf));
		(void)xstrlcat(buf, " ", sizeof(buf));
		(void)xstrlcat(buf, utsname.release, sizeof(buf));
	}

	m->meta.os = xstrdup(buf);
	m->lastnamed = m->lastsec = SEC_BODY;

	return(post_prol(m));
}


static int
post_bl_tagwidth(struct mdoc *m)
{
	struct mdoc_node  *n;
	int		   sz;
	char		   buf[32];

	/*
	 * If -tag has been specified and -width has not been, then try
	 * to intuit our width from the first body element.  
	 */

	if (NULL == (n = m->last->body->child))
		return(1);

	/*
	 * Use the text width, if a text node, or the default macro
	 * width if a macro.
	 */

	if ((n = n->head->child)) {
		if (MDOC_TEXT != n->type) {
			if (0 == (sz = (int)mdoc_macro2len(n->tok)))
				sz = -1;
		} else
			sz = (int)strlen(n->string) + 1;
	} else
		sz = -1;

	if (-1 == sz) {
		if ( ! mwarn(m, WNOWIDTH))
			return(0);
		sz = 10;
	}

	(void)snprintf(buf, sizeof(buf), "%dn", sz);

	/*
	 * We have to dynamically add this to the macro's argument list.
	 * We're guaranteed that a MDOC_Width doesn't already exist.
	 */

	if (NULL == m->last->args) {
		m->last->args = xcalloc
			(1, sizeof(struct mdoc_arg));
		m->last->args->refcnt = 1;
	}

	n = m->last;
	sz = (int)n->args->argc;
	
	(n->args->argc)++;

	n->args->argv = xrealloc(n->args->argv, 
			n->args->argc * sizeof(struct mdoc_arg));

	n->args->argv[sz - 1].arg = MDOC_Width;
	n->args->argv[sz - 1].line = m->last->line;
	n->args->argv[sz - 1].pos = m->last->pos;
	n->args->argv[sz - 1].sz = 1;
	n->args->argv[sz - 1].value = xcalloc(1, sizeof(char *));
	n->args->argv[sz - 1].value[0] = xstrdup(buf);

	return(1);
}


static int
post_bl_width(struct mdoc *m)
{
	size_t		  width;
	int		  i, tok;
	char		  buf[32];
	char		 *p;

	if (NULL == m->last->args)
		return(merr(m, ENOWIDTH));

	for (i = 0; i < (int)m->last->args->argc; i++)
		if (MDOC_Width == m->last->args->argv[i].arg)
			break;

	if (i == (int)m->last->args->argc)
		return(merr(m, ENOWIDTH));

	p = m->last->args->argv[i].value[0];

	/*
	 * If the value to -width is a macro, then we re-write it to be
	 * the macro's width as set in share/tmac/mdoc/doc-common.
	 */

	if (xstrcmp(p, "Ds"))
		width = 8;
	else if (MDOC_MAX == (tok = mdoc_tokhash_find(m->htab, p)))
		return(1);
	else if (0 == (width = mdoc_macro2len(tok))) 
		return(mwarn(m, WNOWIDTH));

	/* The value already exists: free and reallocate it. */

	(void)snprintf(buf, sizeof(buf), "%zun", width);

	free(m->last->args->argv[i].value[0]);
	m->last->args->argv[i].value[0] = xstrdup(buf);

	return(1);
}


static int
post_bl(POST_ARGS)
{
	int		  i, r, len;

	if (MDOC_BLOCK != m->last->type)
		return(1);

	/*
	 * These are fairly complicated, so we've broken them into two
	 * functions.  post_bl_tagwidth() is called when a -tag is
	 * specified, but no -width (it must be guessed).  The second
	 * when a -width is specified (macro indicators must be
	 * rewritten into real lengths).
	 */

	len = (int)(m->last->args ? m->last->args->argc : 0);

	for (r = i = 0; i < len; i++) {
		if (MDOC_Tag == m->last->args->argv[i].arg)
			r |= 1 << 0;
		if (MDOC_Width == m->last->args->argv[i].arg)
			r |= 1 << 1;
	}

	if (r & (1 << 0) && ! (r & (1 << 1))) {
		if ( ! post_bl_tagwidth(m))
			return(0);
	} else if (r & (1 << 1))
		if ( ! post_bl_width(m))
			return(0);

	return(1);
}


static int
post_ar(POST_ARGS)
{
	struct mdoc_node *n;

	if (m->last->child)
		return(1);
	
	n = m->last;
	m->next = MDOC_NEXT_CHILD;
	if ( ! mdoc_word_alloc(m, m->last->line,
				m->last->pos, "file"))
		return(0);
	m->next = MDOC_NEXT_SIBLING;
	if ( ! mdoc_word_alloc(m, m->last->line, 
				m->last->pos, "..."))
		return(0);

	m->last = n;
	m->next = MDOC_NEXT_SIBLING;
	return(1);
}


static int
post_dd(POST_ARGS)
{
	char		  buf[64];

	(void)xstrlcpys(buf, m->last->child, sizeof(buf));

	if (0 == (m->meta.date = mdoc_atotime(buf))) {
		if ( ! mwarn(m, WBADDATE))
			return(0);
		m->meta.date = time(NULL);
	}

	return(post_prol(m));
}


static int
post_prol(POST_ARGS)
{
	struct mdoc_node *n;

	/* 
	 * The end document shouldn't have the prologue macros as part
	 * of the syntax tree (they encompass only meta-data).  
	 */

	if (m->last->parent->child == m->last)
		m->last->parent->child = m->last->prev;
	if (m->last->prev)
		m->last->prev->next = NULL;

	n = m->last;
	assert(NULL == m->last->next);

	if (m->last->prev) {
		m->last = m->last->prev;
		m->next = MDOC_NEXT_SIBLING;
	} else {
		m->last = m->last->parent;
		m->next = MDOC_NEXT_CHILD;
	}

	mdoc_node_freelist(n);
	return(1);
}


static int
pre_bd(PRE_ARGS)
{
	int		 i;

	if (MDOC_BODY != n->type)
		return(1);

	/* 
	 * We ONLY enter a literal context if `Bd -literal' or `Bd
	 * -unfilled'.  
	 */

	n = n->parent;

	for (i = 0; i < (int)n->args->argc; i++)
		if (MDOC_Literal == n->args->argv[i].arg)
			break;
		else if (MDOC_Unfilled == n->args->argv[i].arg)
			break;

	if (i < (int)n->args->argc)
		m->flags |= MDOC_LITERAL;

	return(1);
}


static int
post_bd(POST_ARGS)
{

	if (MDOC_BODY == m->last->type)
		m->flags &= ~MDOC_LITERAL;
	return(1);
}


