/* $Id: termact.c,v 1.1 2009/02/21 19:05:28 kristaps Exp $ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
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

#include "term.h"

#define	TTYPE_PROG	  0
#define	TTYPE_CMD_FLAG	  1
#define	TTYPE_CMD_ARG	  2
#define	TTYPE_SECTION	  3
#define	TTYPE_NMAX	  4

/* 
 * These define "styles" for element types, like command arguments or
 * executable names.  This is useful when multiple macros must decorate
 * the same thing (like .Ex -std cmd and .Nm cmd). 
 */

const	int ttypes[TTYPE_NMAX] = {
	TERMP_BOLD,		/* TTYPE_PROG */
	TERMP_BOLD,		/* TTYPE_CMD_FLAG */
	TERMP_UNDERLINE, 	/* TTYPE_CMD_ARG */
	TERMP_BOLD 		/* TTYPE_SECTION */
};

static	int		  arg_hasattr(int, size_t, 
				const struct mdoc_arg *);
static	int		  arg_getattr(int, size_t, 
				const struct mdoc_arg *);

/*
 * What follows describes prefix and postfix operations for the abstract
 * syntax tree descent.
 */

#define	DECL_ARGS \
	struct termp *p, \
	const struct mdoc_meta *meta, \
	const struct mdoc_node *node

#define	DECL_PRE(name) \
static	int	 	  name##_pre(DECL_ARGS)
#define	DECL_POST(name) \
static	void	 	  name##_post(DECL_ARGS)

DECL_PRE(termp_aq);
DECL_PRE(termp_ar);
DECL_PRE(termp_d1);
DECL_PRE(termp_dq);
DECL_PRE(termp_ex);
DECL_PRE(termp_fl);
DECL_PRE(termp_it);
DECL_PRE(termp_nd);
DECL_PRE(termp_nm);
DECL_PRE(termp_ns);
DECL_PRE(termp_op);
DECL_PRE(termp_pp);
DECL_PRE(termp_sh);
DECL_PRE(termp_ud);
DECL_PRE(termp_xr);

DECL_POST(termp_aq);
DECL_POST(termp_ar);
DECL_POST(termp_bl);
DECL_POST(termp_d1);
DECL_POST(termp_dq);
DECL_POST(termp_fl);
DECL_POST(termp_it);
DECL_POST(termp_nm);
DECL_POST(termp_op);
DECL_POST(termp_sh);

const	struct termact __termacts[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */
	{ NULL, NULL }, /* Dt */
	{ NULL, NULL }, /* Os */
	{ termp_sh_pre, termp_sh_post }, /* Sh */
	{ NULL, NULL }, /* Ss */ 
	{ termp_pp_pre, NULL }, /* Pp */ 
	{ termp_d1_pre, termp_d1_post }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ NULL, NULL }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ NULL, termp_bl_post }, /* Bl */
	{ NULL, NULL }, /* El */
	{ termp_it_pre, termp_it_post }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ termp_ar_pre, termp_ar_post }, /* Ar */
	{ NULL, NULL }, /* Cd */
	{ NULL, NULL }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ termp_ex_pre, NULL }, /* Ex */
	{ NULL, NULL }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ 
	{ termp_fl_pre, termp_fl_post }, /* Fl */
	{ NULL, NULL }, /* Fn */ 
	{ NULL, NULL }, /* Ft */ 
	{ NULL, NULL }, /* Ic */ 
	{ NULL, NULL }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ termp_nd_pre, NULL }, /* Nd */ 
	{ termp_nm_pre, termp_nm_post }, /* Nm */ 
	{ termp_op_pre, termp_op_post }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ NULL, NULL }, /* Rv */
	{ NULL, NULL }, /* St */ 
	{ NULL, NULL }, /* Va */
	{ NULL, NULL }, /* Vt */ 
	{ termp_xr_pre, NULL }, /* Xr */
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
	{ termp_aq_pre, termp_aq_post }, /* Aq */
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
	{ termp_dq_pre, termp_dq_post }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ NULL, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ termp_ns_pre, NULL }, /* Ns */
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
	{ termp_ud_pre, NULL }, /* Ud */
};

const struct termact *termacts = __termacts;


/* ARGSUSED */
static int
termp_dq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);

	word(p, "``");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_dq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;

	p->flags |= TERMP_NOSPACE;
	word(p, "''");
}


/* ARGSUSED */
static void
termp_it_post(DECL_ARGS)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width;

	/*
	 * This (and termp_it_pre()) are the most complicated functions
	 * here.  They must account for a considerable number of
	 * switches that completely change the output behaviour, like
	 * -tag versus -column.  Yech.
	 */

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		break;
	default:
		return;
	}

	it = node->parent;
	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		assert(i >= 0);
		assert(1 == bl->argv[i].sz);
		width = strlen(*bl->argv[i].value); /* XXX */

		if (MDOC_HEAD == node->type) {
			flushln(p);
			/* FIXME: nested lists. */
			p->rmargin = p->maxrmargin;
			p->flags &= ~TERMP_NOBREAK;
		} else {
			flushln(p);
			p->offset -= width + 1;
			p->flags &= ~TERMP_NOLPAD;
		}
	}
}


/* ARGSUSED */
static int
termp_it_pre(DECL_ARGS)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width;

	/*
	 * Also see termp_it_post() for general comments.
	 */

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		it = node->parent;
		break;
	case (MDOC_BLOCK):
		it = node;
		break;
	default:
		return(1);
	}

	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-compact', don't assert vertical space. */

	if (MDOC_BLOCK == node->type) {
		if (arg_hasattr(MDOC_Compact, bl->argc, bl->argv))
			newln(p);
		else
			vspace(p);
		return(1);
	}

	assert(MDOC_HEAD == node->type 
			|| MDOC_BODY == node->type);

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		assert(i >= 0); /* XXX */
		assert(1 == bl->argv[i].sz);
		width = strlen(*bl->argv[i].value); /* XXX */

		/* FIXME: nested lists. */

		if (MDOC_HEAD == node->type) {
			p->flags |= TERMP_NOBREAK;
			p->flags |= TERMP_NOSPACE;
			p->rmargin = p->offset + width;
		} else {
			p->flags |= TERMP_NOSPACE;
			p->flags |= TERMP_NOLPAD;
			p->offset += width + 1;
		}
	}

	return(1);
}


/* ARGSUSED */
static void
termp_nm_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_PROG];
}


/* ARGSUSED */
static void
termp_fl_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD_FLAG];
}


/* ARGSUSED */
static int
termp_ar_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD_ARG];
	if (NULL == node->child)
		word(p, "...");
	return(1);
}


/* ARGSUSED */
static int
termp_nm_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_PROG];
	if (NULL == node->child)
		word(p, meta->name);
	return(1);
}


/* ARGSUSED */
static int
termp_ns_pre(DECL_ARGS)
{

	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_pp_pre(DECL_ARGS)
{

	vspace(p);
	return(1);
}


/* ARGSUSED */
static void
termp_ar_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD_ARG];
}


/* ARGSUSED */
static int
termp_ex_pre(DECL_ARGS)
{
	int		 i;

	i = arg_getattr(MDOC_Std, node->data.elem.argc, 
			node->data.elem.argv);
	assert(i >= 0);

	word(p, "The");
	p->flags |= ttypes[TTYPE_PROG];
	word(p, *node->data.elem.argv[i].value);
	p->flags &= ~ttypes[TTYPE_PROG];
       	word(p, "utility exits 0 on success, and >0 if an error occurs.");

	return(1);
}


/* ARGSUSED */
static int
termp_nd_pre(DECL_ARGS)
{

	word(p, "\\-");
	return(1);
}


/* ARGSUSED */
static void
termp_bl_post(DECL_ARGS)
{

	if (MDOC_BLOCK == node->type)
		newln(p);
}


/* ARGSUSED */
static void
termp_op_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, "\\(rB");
}


/* ARGSUSED */
static void
termp_sh_post(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		p->flags &= ~ttypes[TTYPE_SECTION];
		newln(p);
		break;
	case (MDOC_BODY):
		newln(p);
		p->offset -= 4;
		break;
	default:
		break;
	}
}


/* ARGSUSED */
static int
termp_xr_pre(DECL_ARGS)
{
	const struct mdoc_node *n;

	n = node->child;
	assert(n);

	assert(MDOC_TEXT == n->type);
	word(p, n->data.text.string);

	if (NULL == (n = n->next)) 
		return(0);

	assert(MDOC_TEXT == n->type);
	p->flags |= TERMP_NOSPACE;
	word(p, "(");
	p->flags |= TERMP_NOSPACE;
	word(p, n->data.text.string);
	p->flags |= TERMP_NOSPACE;
	word(p, ")");

	return(0);
}


/* ARGSUSED */
static int
termp_sh_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		vspace(p);
		p->flags |= ttypes[TTYPE_SECTION];
		break;
	case (MDOC_BODY):
		p->offset += 4;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
static int
termp_op_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_BODY):
		word(p, "\\(lB");
		p->flags |= TERMP_NOSPACE;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
static int
termp_ud_pre(DECL_ARGS)
{

	word(p, "currently under development.");
	return(1);
}


/* ARGSUSED */
static int
termp_fl_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD_FLAG];
	word(p, "\\-");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_d1_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	newln(p);
	p->offset += 4;
	return(1);
}


/* ARGSUSED */
static void
termp_d1_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;
	newln(p);
	p->offset -= 4;
}


/* ARGSUSED */
static int
termp_aq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "\\<");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_aq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, "\\>");
}


static int
arg_hasattr(int arg, size_t argc, const struct mdoc_arg *argv)
{

	return(-1 != arg_getattr(arg, argc, argv));
}


static int
arg_getattr(int arg, size_t argc, const struct mdoc_arg *argv)
{
	int		 i;

	for (i = 0; i < (int)argc; i++) 
		if (argv[i].arg == arg)
			return(i);
	return(-1);
}

