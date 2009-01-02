/* $Id: argv.c,v 1.5 2009/01/01 20:40:16 kristaps Exp $ */
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


static	int		 lookup(int, const char *);
static	int		 parse(struct mdoc *, int, 
				struct mdoc_arg *, int *, char *);
static	int		 postparse(struct mdoc *, int, 
				const struct mdoc_arg *, int);


int
mdoc_args(struct mdoc *mdoc, int tok, int *pos, char *buf, int fl, char **v)
{
	int		 i;

	if (0 == buf[*pos])
		return(ARGS_EOLN);

	if ('\"' == buf[*pos] && ! (fl & ARGS_QUOTED))
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_QUOTED))
			return(ARGS_ERROR);

	if ('-' == buf[*pos]) 
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_ARGLIKE))
			return(ARGS_ERROR);

	if ((fl & ARGS_DELIM) && mdoc_iscdelim(buf[*pos])) {
		for (i = *pos; buf[i]; ) {
			if ( ! mdoc_iscdelim(buf[i]))
				break;
			i++;
			if (0 == buf[i] || ! isspace(buf[i]))
				break;
			i++;
			while (buf[i] && isspace(buf[i]))
				i++;
		}
		if (0 == buf[i]) {
			*v = &buf[*pos];
			return(ARGS_PUNCT);
		}
	}

	/*
	 * Parse routine for non-quoted string.  
	 */

	if ('\"' != buf[*pos]) {
		*v = &buf[*pos];

		while (buf[*pos] && ! isspace(buf[*pos]))
			(*pos)++;

		if (0 == buf[*pos])
			return(ARGS_WORD);

		buf[(*pos)++] = 0;
		if (0 == buf[*pos])
			return(ARGS_WORD);

		while (buf[*pos] && isspace(buf[*pos]))
			(*pos)++;

		if (buf[*pos])
			return(ARGS_WORD);

		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
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
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_UNQUOTE);
		return(ARGS_ERROR);
	}

	buf[(*pos)++] = 0;
	if (0 == buf[*pos])
		return(ARGS_WORD);

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		return(ARGS_WORD);

	if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
		return(ARGS_ERROR);

	return(ARGS_WORD);
}


static int
lookup(int tok, const char *argv)
{

	switch (tok) {
	case (MDOC_Bf):
		if (xstrcmp(argv, "emphasis"))
			return(MDOC_Emphasis);
		else if (xstrcmp(argv, "literal"))
			return(MDOC_Literal);
		else if (xstrcmp(argv, "symbolic"))
			return(MDOC_Symbolic);
		break;

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
		else if (xstrcmp(argv, "literal"))
			return(MDOC_Literal);
		else if (xstrcmp(argv, "file"))
			return(MDOC_File);
		else if (xstrcmp(argv, "offset"))
			return(MDOC_Offset);
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
	
	case (MDOC_St):
		if (xstrcmp(argv, "p1003.1-88"))
			return(MDOC_p1003_1_88);
		else if (xstrcmp(argv, "p1003.1-90"))
			return(MDOC_p1003_1_90);
		else if (xstrcmp(argv, "p1003.1-96"))
			return(MDOC_p1003_1_96);
		else if (xstrcmp(argv, "p1003.1-2001"))
			return(MDOC_p1003_1_2001);
		else if (xstrcmp(argv, "p1003.1-2004"))
			return(MDOC_p1003_1_2004);
		else if (xstrcmp(argv, "p1003.1"))
			return(MDOC_p1003_1);
		else if (xstrcmp(argv, "p1003.1b"))
			return(MDOC_p1003_1b);
		else if (xstrcmp(argv, "p1003.1b-93"))
			return(MDOC_p1003_1b_93);
		else if (xstrcmp(argv, "p1003.1c-95"))
			return(MDOC_p1003_1c_95);
		else if (xstrcmp(argv, "p1003.1g-2000"))
			return(MDOC_p1003_1g_2000);
		else if (xstrcmp(argv, "p1003.2-92"))
			return(MDOC_p1003_2_92);
		else if (xstrcmp(argv, "p1003.2-95"))
			return(MDOC_p1387_2_95);
		else if (xstrcmp(argv, "p1003.2"))
			return(MDOC_p1003_2);
		else if (xstrcmp(argv, "p1387.2-95"))
			return(MDOC_p1387_2);
		else if (xstrcmp(argv, "isoC-90"))
			return(MDOC_isoC_90);
		else if (xstrcmp(argv, "isoC-amd1"))
			return(MDOC_isoC_amd1);
		else if (xstrcmp(argv, "isoC-tcor1"))
			return(MDOC_isoC_tcor1);
		else if (xstrcmp(argv, "isoC-tcor2"))
			return(MDOC_isoC_tcor2);
		else if (xstrcmp(argv, "isoC-99"))
			return(MDOC_isoC_99);
		else if (xstrcmp(argv, "ansiC"))
			return(MDOC_ansiC);
		else if (xstrcmp(argv, "ansiC-89"))
			return(MDOC_ansiC_89);
		else if (xstrcmp(argv, "ansiC-99"))
			return(MDOC_ansiC_99);
		else if (xstrcmp(argv, "ieee754"))
			return(MDOC_ieee754);
		else if (xstrcmp(argv, "iso8802-3"))
			return(MDOC_iso8802_3);
		else if (xstrcmp(argv, "xpg3"))
			return(MDOC_xpg3);
		else if (xstrcmp(argv, "xpg4"))
			return(MDOC_xpg4);
		else if (xstrcmp(argv, "xpg4.2"))
			return(MDOC_xpg4_2);
		else if (xstrcmp(argv, "xpg4.3"))
			return(MDOC_xpg4_3);
		else if (xstrcmp(argv, "xbd5"))
			return(MDOC_xbd5);
		else if (xstrcmp(argv, "xcu5"))
			return(MDOC_xcu5);
		else if (xstrcmp(argv, "xsh5"))
			return(MDOC_xsh5);
		else if (xstrcmp(argv, "xns5"))
			return(MDOC_xns5);
		else if (xstrcmp(argv, "xns5.2d2.0"))
			return(MDOC_xns5_2d2_0);
		else if (xstrcmp(argv, "xcurses4.2"))
			return(MDOC_xcurses4_2);
		else if (xstrcmp(argv, "susv2"))
			return(MDOC_susv2);
		else if (xstrcmp(argv, "susv3"))
			return(MDOC_susv3);
		else if (xstrcmp(argv, "svid4"))
			return(MDOC_svid4);
		break;

	default:
		abort();
		/* NOTREACHED */
	}

	return(MDOC_ARG_MAX);
}


static int
postparse(struct mdoc *mdoc, int tok, const struct mdoc_arg *v, int pos)
{

	switch (v->arg) {
	case (MDOC_Offset):
		assert(v->value);
		assert(v->value[0]);
		if (xstrcmp(v->value[0], "left"))
			break;
		if (xstrcmp(v->value[0], "right"))
			break;
		if (xstrcmp(v->value[0], "center"))
			break;
		if (xstrcmp(v->value[0], "indent"))
			break;
		if (xstrcmp(v->value[0], "indent-two"))
			break;
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_ARGBAD));
	default:
		break;
	}

	return(1);
}


static int
parse(struct mdoc *mdoc, int tok, 
		struct mdoc_arg *v, int *pos, char *buf)
{
	char		*p;
	int		 c, ppos, i;

	ppos = *pos;

	switch (v->arg) {
	case(MDOC_Std):
		/* FALLTHROUGH */
	case(MDOC_Width):
		/* FALLTHROUGH */
	case(MDOC_Offset):
		/*
		 * This has a single value for an argument.
		 */
		c = mdoc_args(mdoc, tok, pos, buf, ARGS_QUOTED, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGVAL));
			
		v->sz = 1;
		v->value = xcalloc(1, sizeof(char *));
		v->value[0] = p;
		break;

	case(MDOC_Column):
		/*
		 * This has several value for a single argument.  We
		 * pre-allocate a pointer array and don't let it exceed
		 * this size.
		 */
		v->sz = 0;
		v->value = xcalloc(MDOC_LINEARG_MAX, sizeof(char *));
		for (i = 0; i < MDOC_LINEARG_MAX; i++) {
			c = mdoc_args(mdoc, tok, pos, buf, ARGS_QUOTED, &p);
			if (ARGS_ERROR == c) {
				free(v->value);
				return(0);
			} else if (ARGS_EOLN == c)
				break;
			v->value[i] = p;
		}
		if (0 == i) {
			free(v->value);
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGVAL));
		} else if (MDOC_LINEARG_MAX == i)
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGMANY));

		v->sz = i;
		break;

	default:
		v->sz = 0;
		v->value = NULL;
		break;
	}

	return(1);
}


int
mdoc_argv(struct mdoc *mdoc, int tok, 
		struct mdoc_arg *v, int *pos, char *buf)
{
	int		 i, ppos;
	char		*argv;

	(void)memset(v, 0, sizeof(struct mdoc_arg));

	if (0 == buf[*pos])
		return(ARGV_EOLN);

	assert( ! isspace(buf[*pos]));

	if ('-' != buf[*pos])
		return(ARGV_WORD);

	i = *pos;
	argv = &buf[++(*pos)];

	while (buf[*pos] && ! isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		buf[(*pos)++] = 0;

	if (MDOC_ARG_MAX == (v->arg = lookup(tok, argv))) {
		(void)mdoc_err(mdoc, tok, i, ERR_SYNTAX_ARG);
		return(ARGV_ERROR);
	}

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	/* FIXME: whitespace if no value. */

	ppos = *pos;
	if ( ! parse(mdoc, tok, v, pos, buf))
		return(ARGV_ERROR);
	if ( ! postparse(mdoc, tok, v, ppos))
		return(ARGV_ERROR);

	return(ARGV_ARG);
}


void
mdoc_argv_free(int sz, struct mdoc_arg *arg)
{
	int		 i;

	for (i = 0; i < sz; i++) {
		if (0 == arg[i].sz) {
			assert(NULL == arg[i].value);
			continue;
		}
		assert(arg[i].value);
		free(arg[i].value);
	}
}

