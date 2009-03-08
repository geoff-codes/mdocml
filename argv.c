/* $Id: argv.c,v 1.42 2009/03/08 20:50:12 kristaps Exp $ */
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

#include "private.h"

/*
 * Routines to parse arguments of macros.  Arguments follow the syntax
 * of `-arg [val [valN...]]'.  Arguments come in all types:  quoted
 * arguments, multiple arguments per value, no-value arguments, etc.
 *
 * There's no limit to the number or arguments that may be allocated.
 */

#define	ARGS_QUOTED	(1 << 0)
#define	ARGS_DELIM	(1 << 1)
#define	ARGS_TABSEP	(1 << 2)
#define	ARGS_ARGVLIKE	(1 << 3)

#define	ARGV_NONE	(1 << 0)
#define	ARGV_SINGLE	(1 << 1)
#define	ARGV_MULTI	(1 << 2)
#define	ARGV_OPT_SINGLE	(1 << 3)

enum 	mwarn {
	WQUOTPARM,
	WARGVPARM,
	WCOLEMPTY,
	WTAILWS	
};

enum	merr {
	EQUOTTERM,
	EARGVAL	
};

static	int		 argv_a2arg(int, const char *);
static	int		 args(struct mdoc *, int, int *, 
				char *, int, char **);
static	int		 argv(struct mdoc *, int, int,
				struct mdoc_argv *, int *, char *);
static	int		 argv_single(struct mdoc *, int, 
				struct mdoc_argv *, int *, char *);
static	int		 argv_opt_single(struct mdoc *, int, 
				struct mdoc_argv *, int *, char *);
static	int		 argv_multi(struct mdoc *, int, 
				struct mdoc_argv *, int *, char *);
static	int		 pwarn(struct mdoc *, int, int, enum mwarn);
static	int		 perr(struct mdoc *, int, int, enum merr);

/* Per-argument flags. */

static	int mdoc_argvflags[MDOC_ARG_MAX] = {
	ARGV_NONE,	/* MDOC_Split */
	ARGV_NONE,	/* MDOC_Nosplit */
	ARGV_NONE,	/* MDOC_Ragged */
	ARGV_NONE,	/* MDOC_Unfilled */
	ARGV_NONE,	/* MDOC_Literal */
	ARGV_NONE,	/* MDOC_File */
	ARGV_SINGLE,	/* MDOC_Offset */
	ARGV_NONE,	/* MDOC_Bullet */
	ARGV_NONE,	/* MDOC_Dash */
	ARGV_NONE,	/* MDOC_Hyphen */
	ARGV_NONE,	/* MDOC_Item */
	ARGV_NONE,	/* MDOC_Enum */
	ARGV_NONE,	/* MDOC_Tag */
	ARGV_NONE,	/* MDOC_Diag */
	ARGV_NONE,	/* MDOC_Hang */
	ARGV_NONE,	/* MDOC_Ohang */
	ARGV_NONE,	/* MDOC_Inset */
	ARGV_MULTI,	/* MDOC_Column */
	ARGV_SINGLE,	/* MDOC_Width */
	ARGV_NONE,	/* MDOC_Compact */
	ARGV_SINGLE,	/* MDOC_Std */
	ARGV_NONE,	/* MDOC_Filled */
	ARGV_NONE,	/* MDOC_Words */
	ARGV_NONE,	/* MDOC_Emphasis */
	ARGV_NONE	/* MDOC_Symbolic */
};

static	int mdoc_argflags[MDOC_MAX] = {
	0, /* \" */
	0, /* Dd */
	0, /* Dt */
	0, /* Os */
	0, /* Sh */
	0, /* Ss */ 
	ARGS_DELIM, /* Pp */ 
	ARGS_DELIM, /* D1 */
	ARGS_DELIM, /* Dl */
	0, /* Bd */
	0, /* Ed */
	0, /* Bl */
	0, /* El */
	0, /* It */
	ARGS_DELIM, /* Ad */ 
	ARGS_DELIM, /* An */
	ARGS_DELIM, /* Ar */
	ARGS_QUOTED, /* Cd */
	ARGS_DELIM, /* Cm */
	ARGS_DELIM, /* Dv */ 
	ARGS_DELIM, /* Er */ 
	ARGS_DELIM, /* Ev */ 
	0, /* Ex */
	ARGS_DELIM | ARGS_QUOTED, /* Fa */ 
	0, /* Fd */ 
	ARGS_DELIM, /* Fl */
	ARGS_DELIM | ARGS_QUOTED, /* Fn */ 
	ARGS_DELIM | ARGS_QUOTED, /* Ft */ 
	ARGS_DELIM, /* Ic */ 
	0, /* In */ 
	ARGS_DELIM, /* Li */
	0, /* Nd */ 
	ARGS_DELIM, /* Nm */ 
	ARGS_DELIM, /* Op */
	0, /* Ot */
	ARGS_DELIM, /* Pa */
	0, /* Rv */
	ARGS_DELIM | ARGS_ARGVLIKE, /* St */ 
	ARGS_DELIM, /* Va */
	ARGS_DELIM, /* Vt */ 
	ARGS_DELIM, /* Xr */
	ARGS_QUOTED, /* %A */
	ARGS_QUOTED, /* %B */
	ARGS_QUOTED, /* %D */
	ARGS_QUOTED, /* %I */
	ARGS_QUOTED, /* %J */
	ARGS_QUOTED, /* %N */
	ARGS_QUOTED, /* %O */
	ARGS_QUOTED, /* %P */
	ARGS_QUOTED, /* %R */
	ARGS_QUOTED, /* %T */
	ARGS_QUOTED, /* %V */
	ARGS_DELIM, /* Ac */
	0, /* Ao */
	ARGS_DELIM, /* Aq */
	ARGS_DELIM, /* At */
	ARGS_DELIM, /* Bc */
	0, /* Bf */ 
	0, /* Bo */
	ARGS_DELIM, /* Bq */
	ARGS_DELIM, /* Bsx */
	ARGS_DELIM, /* Bx */
	0, /* Db */
	ARGS_DELIM, /* Dc */
	0, /* Do */
	ARGS_DELIM, /* Dq */
	ARGS_DELIM, /* Ec */
	0, /* Ef */
	ARGS_DELIM, /* Em */ 
	0, /* Eo */
	ARGS_DELIM, /* Fx */
	ARGS_DELIM, /* Ms */
	ARGS_DELIM, /* No */
	ARGS_DELIM, /* Ns */
	ARGS_DELIM, /* Nx */
	ARGS_DELIM, /* Ox */
	ARGS_DELIM, /* Pc */
	ARGS_DELIM, /* Pf */
	0, /* Po */
	ARGS_DELIM, /* Pq */
	ARGS_DELIM, /* Qc */
	ARGS_DELIM, /* Ql */
	0, /* Qo */
	ARGS_DELIM, /* Qq */
	0, /* Re */
	0, /* Rs */
	ARGS_DELIM, /* Sc */
	0, /* So */
	ARGS_DELIM, /* Sq */
	0, /* Sm */
	ARGS_DELIM, /* Sx */
	ARGS_DELIM, /* Sy */
	ARGS_DELIM, /* Tn */
	ARGS_DELIM, /* Ux */
	ARGS_DELIM, /* Xc */
	0, /* Xo */
	0, /* Fo */ 
	0, /* Fc */ 
	0, /* Oo */
	ARGS_DELIM, /* Oc */
	0, /* Bk */
	0, /* Ek */
	0, /* Bt */
	0, /* Hf */
	0, /* Fr */
	0, /* Ud */
	0, /* Lb */
	0, /* Ap */
	0, /* Lp */
};


/*
 * Parse an argument from line text.  This comes in the form of -key
 * [value0...], which may either have a single mandatory value, at least
 * one mandatory value, an optional single value, or no value.
 */
int
mdoc_argv(struct mdoc *mdoc, int line, int tok,
		struct mdoc_arg **v, int *pos, char *buf)
{
	int		  i;
	char		 *p;
	struct mdoc_argv tmp;
	struct mdoc_arg	 *arg;

	if (0 == buf[*pos])
		return(ARGV_EOLN);

	assert( ! isspace((u_char)buf[*pos]));

	if ('-' != buf[*pos])
		return(ARGV_WORD);

	i = *pos;
	p = &buf[++(*pos)];

	assert(*pos > 0);

	/* LINTED */
	while (buf[*pos]) {
		if (isspace((u_char)buf[*pos])) 
			if ('\\' != buf[*pos - 1])
				break;
		(*pos)++;
	}

	if (buf[*pos])
		buf[(*pos)++] = 0;

	(void)memset(&tmp, 0, sizeof(struct mdoc_argv));
	tmp.line = line;
	tmp.pos = *pos;

	/*
	 * We now parse out the per-macro arguments.  XXX - this can be
	 * made much cleaner using per-argument tables.  See argv_a2arg
	 * for details.
	 */

	if (MDOC_ARG_MAX == (tmp.arg = argv_a2arg(tok, p))) {
		if ( ! pwarn(mdoc, line, i, WARGVPARM))
			return(ARGV_ERROR);
		return(ARGV_WORD);
	}

	while (buf[*pos] && isspace((u_char)buf[*pos]))
		(*pos)++;

	/* FIXME: whitespace if no value. */

	if ( ! argv(mdoc, tok, line, &tmp, pos, buf))
		return(ARGV_ERROR);

	if (NULL == (arg = *v)) {
		*v = xcalloc(1, sizeof(struct mdoc_arg));
		arg = *v;
	} 

	arg->argc++;
	arg->argv = xrealloc(arg->argv, arg->argc * 
			sizeof(struct mdoc_argv));

	(void)memcpy(&arg->argv[(int)arg->argc - 1], 
			&tmp, sizeof(struct mdoc_argv));

	return(ARGV_ARG);
}


void
mdoc_argv_free(struct mdoc_arg *p)
{
	int		 i, j;

	if (NULL == p)
		return;

	if (p->refcnt) {
		--(p->refcnt);
		if (p->refcnt)
			return;
	}

	assert(p->argc);

	/* LINTED */
	for (i = 0; i < (int)p->argc; i++) {
		if (0 == p->argv[i].sz)
			continue;
		/* LINTED */
		for (j = 0; j < (int)p->argv[i].sz; j++)
			free(p->argv[i].value[j]);

		free(p->argv[i].value);
	}

	free(p->argv);
	free(p);
}



static int
perr(struct mdoc *mdoc, int line, int pos, enum merr code)
{
	char		*p;

	p = NULL;

	switch (code) {
	case (EQUOTTERM):
		p = "unterminated quoted parameter";
		break;
	case (EARGVAL):
		p = "argument requires a value";
		break;
	}

	assert(p);
	return(mdoc_perr(mdoc, line, pos, p));
}


static int
pwarn(struct mdoc *mdoc, int line, int pos, enum mwarn code)
{
	char		*p;
	int		 c;

	p = NULL;
	c = WARN_SYNTAX;

	switch (code) {
	case (WQUOTPARM):
		p = "unexpected quoted parameter";
		break;
	case (WARGVPARM):
		p = "argument-like parameter";
		break;
	case (WCOLEMPTY):
		p = "last list column is empty";
		c = WARN_COMPAT;
		break;
	case (WTAILWS):
		p = "trailing whitespace";
		c = WARN_COMPAT;
		break;
	}

	assert(p);
	return(mdoc_pwarn(mdoc, line, pos, c, p));
}


int
mdoc_args(struct mdoc *mdoc, int line, 
		int *pos, char *buf, int tok, char **v)
{
	int		  fl, c, i;
	struct mdoc_node *n;

	fl = (0 == tok) ? 0 : mdoc_argflags[tok];

	/* 
	 * Override per-macro argument flags with context-specific ones.
	 * As of now, this is only valid for `It' depending on its list
	 * context.
	 */

	switch (tok) {
	case (MDOC_It):
		for (n = mdoc->last; n; n = n->parent)
			if (MDOC_BLOCK == n->type && MDOC_Bl == n->tok)
				break;

		assert(n);
		c = (int)(n->args ? n->args->argc : 0);
		assert(c > 0);

		/*
		 * Using `Bl -column' adds ARGS_TABSEP to the arguments
		 * and invalidates ARGS_DELIM.  Using `Bl -diag' allows
		 * for quoted arguments.
		 */

		/* LINTED */
		for (i = 0; i < c; i++) {
			switch (n->args->argv[i].arg) {
			case (MDOC_Column):
				fl |= ARGS_TABSEP;
				fl &= ~ARGS_DELIM;
				i = c;
				break;
			case (MDOC_Diag):
				fl |= ARGS_QUOTED;
				i = c;
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	/* Continue parsing the arguments themselves...  */

	return(args(mdoc, line, pos, buf, fl, v));
}


static int
args(struct mdoc *mdoc, int line, 
		int *pos, char *buf, int fl, char **v)
{
	int		  i;
	char		 *p, *pp;

	assert(*pos > 0);

	if (0 == buf[*pos])
		return(ARGS_EOLN);

	if ('\"' == buf[*pos] && ! (fl & ARGS_QUOTED))
		if ( ! pwarn(mdoc, line, *pos, WQUOTPARM))
			return(ARGS_ERROR);

	if ( ! (fl & ARGS_ARGVLIKE) && '-' == buf[*pos]) 
		if ( ! pwarn(mdoc, line, *pos, WARGVPARM))
			return(ARGS_ERROR);

	/* 
	 * If the first character is a delimiter and we're to look for
	 * delimited strings, then pass down the buffer seeing if it
	 * follows the pattern of [[::delim::][ ]+]+.
	 */

	if ((fl & ARGS_DELIM) && mdoc_iscdelim(buf[*pos])) {
		for (i = *pos; buf[i]; ) {
			if ( ! mdoc_iscdelim(buf[i]))
				break;
			i++;
			/* There must be at least one space... */
			if (0 == buf[i] || ! isspace((u_char)buf[i]))
				break;
			i++;
			while (buf[i] && isspace((u_char)buf[i]))
				i++;
		}
		if (0 == buf[i]) {
			*v = &buf[*pos];
			return(ARGS_PUNCT);
		}
	}

	/* First parse non-quoted strings. */

	if ('\"' != buf[*pos] || ! (ARGS_QUOTED & fl)) {
		*v = &buf[*pos];

		/* 
		 * Thar be dragons here!  If we're tab-separated, search
		 * ahead for either a tab or the `Ta' macro.  If a tab
		 * is detected, it mustn't be escaped; if a `Ta' is
		 * detected, it must be space-buffered before and after.
		 * If either of these hold true, then prune out the
		 * extra spaces and call it an argument.
		 */

		if (ARGS_TABSEP & fl) {
			/* Scan ahead to unescaped tab. */

			for (p = *v; ; p++) {
				if (NULL == (p = strchr(p, '\t')))
					break;
				if (p == *v)
					break;
				if ('\\' != *(p - 1))
					break;
			}

			/* Scan ahead to unescaped `Ta'. */

			for (pp = *v; ; pp++) {
				if (NULL == (pp = strstr(pp, "Ta")))
					break;
				if (pp > *v && ' ' != *(pp - 1))
					continue;
				if (' ' == *(pp + 2) || 0 == *(pp + 2))
					break;
			}

			/* Choose delimiter tab/Ta. */

			if (p && pp)
				p = (p < pp ? p : pp);
			else if ( ! p && pp)
				p = pp;

			/* Strip delimiter's preceding whitespace. */

			if (p && p > *v) {
				pp = p - 1;
				while (pp > *v && ' ' == *pp)
					pp--;
				if (pp == *v && ' ' == *pp) 
					*pp = 0;
				else if (' ' == *pp)
					*(pp + 1) = 0;
			}

			/* ...in- and proceding whitespace. */

			if (p && ('\t' != *p)) {
				*p++ = 0;
				*p++ = 0;
			} else if (p)
				*p++ = 0;

			if (p) {
				while (' ' == *p)
					p++;
				if (0 != *p)
					*(p - 1) = 0;
				*pos += (int)(p - *v);
			} 

			if (p && 0 == *p)
				if ( ! pwarn(mdoc, line, *pos, WCOLEMPTY))
					return(0);
			if (p && 0 == *p && p > *v && ' ' == *(p - 1))
				if ( ! pwarn(mdoc, line, *pos, WTAILWS))
					return(0);

			if (p)
				return(ARGS_WORD);

			/* Configure the eoln case, too. */

			p = strchr(*v, 0);
			assert(p);

			if (p > *v && ' ' == *(p - 1))
				if ( ! pwarn(mdoc, line, *pos, WTAILWS))
					return(0);
			*pos += (int)(p - *v);

			return(ARGS_WORD);
		} 

		/* Do non-tabsep look-ahead here. */
		
		if ( ! (ARGS_TABSEP & fl))
			while (buf[*pos]) {
				if (isspace((u_char)buf[*pos]))
					if ('\\' != buf[*pos - 1])
						break;
				(*pos)++;
			}

		if (0 == buf[*pos])
			return(ARGS_WORD);

		buf[(*pos)++] = 0;

		if (0 == buf[*pos])
			return(ARGS_WORD);

		if ( ! (ARGS_TABSEP & fl))
			while (buf[*pos] && isspace((u_char)buf[*pos]))
				(*pos)++;

		if (buf[*pos])
			return(ARGS_WORD);

		if ( ! pwarn(mdoc, line, *pos, WTAILWS))
			return(ARGS_ERROR);

		return(ARGS_WORD);
	}

	/*
	 * If we're a quoted string (and quoted strings are allowed),
	 * then parse ahead to the next quote.  If none's found, it's an
	 * error.  After, parse to the next word.  
	 */

	*v = &buf[++(*pos)];

	while (buf[*pos] && '\"' != buf[*pos])
		(*pos)++;

	if (0 == buf[*pos]) {
		(void)perr(mdoc, line, *pos, EQUOTTERM);
		return(ARGS_ERROR);
	}

	buf[(*pos)++] = 0;
	if (0 == buf[*pos])
		return(ARGS_QWORD);

	while (buf[*pos] && isspace((u_char)buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		return(ARGS_QWORD);

	if ( ! pwarn(mdoc, line, *pos, WTAILWS))
		return(ARGS_ERROR);

	return(ARGS_QWORD);
}


static int
argv_a2arg(int tok, const char *argv)
{

	/*
	 * Parse an argument identifier from its text.  XXX - this
	 * should really be table-driven to clarify the code.
	 *
	 * If you add an argument to the list, make sure that you
	 * register it here with its one or more macros!
	 */

	switch (tok) {
	case (MDOC_An):
		if (xstrcmp(argv, "split"))
			return(MDOC_Split);
		else if (xstrcmp(argv, "nosplit"))
			return(MDOC_Nosplit);
		break;

	case (MDOC_Bd):
		if (xstrcmp(argv, "ragged"))
			return(MDOC_Ragged);
		else if (xstrcmp(argv, "unfilled"))
			return(MDOC_Unfilled);
		else if (xstrcmp(argv, "filled"))
			return(MDOC_Filled);
		else if (xstrcmp(argv, "literal"))
			return(MDOC_Literal);
		else if (xstrcmp(argv, "file"))
			return(MDOC_File);
		else if (xstrcmp(argv, "offset"))
			return(MDOC_Offset);
		break;

	case (MDOC_Bf):
		if (xstrcmp(argv, "emphasis"))
			return(MDOC_Emphasis);
		else if (xstrcmp(argv, "literal"))
			return(MDOC_Literal);
		else if (xstrcmp(argv, "symbolic"))
			return(MDOC_Symbolic);
		break;

	case (MDOC_Bk):
		if (xstrcmp(argv, "words"))
			return(MDOC_Words);
		break;

	case (MDOC_Bl):
		if (xstrcmp(argv, "bullet"))
			return(MDOC_Bullet);
		else if (xstrcmp(argv, "dash"))
			return(MDOC_Dash);
		else if (xstrcmp(argv, "hyphen"))
			return(MDOC_Hyphen);
		else if (xstrcmp(argv, "item"))
			return(MDOC_Item);
		else if (xstrcmp(argv, "enum"))
			return(MDOC_Enum);
		else if (xstrcmp(argv, "tag"))
			return(MDOC_Tag);
		else if (xstrcmp(argv, "diag"))
			return(MDOC_Diag);
		else if (xstrcmp(argv, "hang"))
			return(MDOC_Hang);
		else if (xstrcmp(argv, "ohang"))
			return(MDOC_Ohang);
		else if (xstrcmp(argv, "inset"))
			return(MDOC_Inset);
		else if (xstrcmp(argv, "column"))
			return(MDOC_Column);
		else if (xstrcmp(argv, "width"))
			return(MDOC_Width);
		else if (xstrcmp(argv, "offset"))
			return(MDOC_Offset);
		else if (xstrcmp(argv, "compact"))
			return(MDOC_Compact);
		break;
	
	case (MDOC_Rv):
		/* FALLTHROUGH */
	case (MDOC_Ex):
		if (xstrcmp(argv, "std"))
			return(MDOC_Std);
		break;
	default:
		break;
	}

	return(MDOC_ARG_MAX);
}


static int
argv_multi(struct mdoc *mdoc, int line, 
		struct mdoc_argv *v, int *pos, char *buf)
{
	int		 c, ppos;
	char		*p;

	ppos = *pos;

	for (v->sz = 0; ; v->sz++) {
		if ('-' == buf[*pos])
			break;
		c = args(mdoc, line, pos, buf, ARGS_QUOTED, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			break;

		if (0 == v->sz % 5)
			v->value = xrealloc(v->value, 
				(v->sz + 5) * sizeof(char *));

		v->value[(int)v->sz] = xstrdup(p);
	}

	if (v->sz)
		return(1);

	return(perr(mdoc, line, ppos, EARGVAL));
}


static int
argv_opt_single(struct mdoc *mdoc, int line, 
		struct mdoc_argv *v, int *pos, char *buf)
{
	int		 c;
	char		*p;

	if ('-' == buf[*pos])
		return(1);

	c = args(mdoc, line, pos, buf, ARGS_QUOTED, &p);
	if (ARGS_ERROR == c)
		return(0);
	if (ARGS_EOLN == c)
		return(1);

	v->sz = 1;
	v->value = xcalloc(1, sizeof(char *));
	v->value[0] = xstrdup(p);
	return(1);
}


/*
 * Parse a single, mandatory value from the stream.
 */
static int
argv_single(struct mdoc *mdoc, int line, 
		struct mdoc_argv *v, int *pos, char *buf)
{
	int		 c, ppos;
	char		*p;

	ppos = *pos;

	c = args(mdoc, line, pos, buf, ARGS_QUOTED, &p);
	if (ARGS_ERROR == c)
		return(0);
	if (ARGS_EOLN == c)
		return(perr(mdoc, line, ppos, EARGVAL));

	v->sz = 1;
	v->value = xcalloc(1, sizeof(char *));
	v->value[0] = xstrdup(p);
	return(1);
}


/*
 * Determine rules for parsing arguments.  Arguments can either accept
 * no parameters, an optional single parameter, one parameter, or
 * multiple parameters.
 */
static int
argv(struct mdoc *mdoc, int tok, int line, 
		struct mdoc_argv *v, int *pos, char *buf)
{
	int		 fl;

	v->sz = 0;
	v->value = NULL;
	fl = mdoc_argvflags[v->arg];

	/*
	 * Override the default per-argument value.
	 */

	switch (tok) {
	case (MDOC_Ex):
		fl = ARGV_OPT_SINGLE;
		break;
	default:
		break;
	}

	switch (fl) {
	case (ARGV_SINGLE):
		return(argv_single(mdoc, line, v, pos, buf));
	case (ARGV_MULTI):
		return(argv_multi(mdoc, line, v, pos, buf));
	case (ARGV_OPT_SINGLE):
		return(argv_opt_single(mdoc, line, v, pos, buf));
	default:
		/* ARGV_NONE */
		break;
	}

	return(1);
}
