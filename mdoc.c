/* $Id: mdoc.c,v 1.51 2009/03/05 13:12:12 kristaps Exp $ */
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "private.h"

/*
 * Main caller in the libmdoc library.  This begins the parsing routine,
 * handles allocation of data, and so forth.  Most of the "work" is done
 * in macro.c and validate.c.
 */

static	struct mdoc_arg	 *argdup(size_t, const struct mdoc_arg *);
static	void		  argfree(size_t, struct mdoc_arg *);
static	void	  	  argcpy(struct mdoc_arg *, 
				const struct mdoc_arg *);

static	struct mdoc_node *mdoc_node_alloc(const struct mdoc *);
static	int		  mdoc_node_append(struct mdoc *, 
				struct mdoc_node *);
static	void		  mdoc_elem_free(struct mdoc_elem *);
static	void		  mdoc_text_free(struct mdoc_text *);


const	char *const __mdoc_macronames[MDOC_MAX] = {		 
	"\\\"",		"Dd",		"Dt",		"Os",
	"Sh",		"Ss",		"Pp",		"D1",
	"Dl",		"Bd",		"Ed",		"Bl",
	"El",		"It",		"Ad",		"An",
	"Ar",		"Cd",		"Cm",		"Dv",
	"Er",		"Ev",		"Ex",		"Fa",
	"Fd",		"Fl",		"Fn",		"Ft",
	"Ic",		"In",		"Li",		"Nd",
	"Nm",		"Op",		"Ot",		"Pa",
	"Rv",		"St",		"Va",		"Vt",
	/* LINTED */
	"Xr",		"\%A",		"\%B",		"\%D",
	/* LINTED */
	"\%I",		"\%J",		"\%N",		"\%O",
	/* LINTED */
	"\%P",		"\%R",		"\%T",		"\%V",
	"Ac",		"Ao",		"Aq",		"At",
	"Bc",		"Bf",		"Bo",		"Bq",
	"Bsx",		"Bx",		"Db",		"Dc",
	"Do",		"Dq",		"Ec",		"Ef",
	"Em",		"Eo",		"Fx",		"Ms",
	"No",		"Ns",		"Nx",		"Ox",
	"Pc",		"Pf",		"Po",		"Pq",
	"Qc",		"Ql",		"Qo",		"Qq",
	"Re",		"Rs",		"Sc",		"So",
	"Sq",		"Sm",		"Sx",		"Sy",
	"Tn",		"Ux",		"Xc",		"Xo",
	"Fo",		"Fc",		"Oo",		"Oc",
	"Bk",		"Ek",		"Bt",		"Hf",
	"Fr",		"Ud",		"Lb",
	};

const	char *const __mdoc_argnames[MDOC_ARG_MAX] = {		 
	"split",		"nosplit",		"ragged",
	"unfilled",		"literal",		"file",		 
	"offset",		"bullet",		"dash",		 
	"hyphen",		"item",			"enum",		 
	"tag",			"diag",			"hang",		 
	"ohang",		"inset",		"column",	 
	"width",		"compact",		"std",	 
	"filled",		"words",		"emphasis",
	"symbolic"
	};

const	char * const *mdoc_macronames = __mdoc_macronames;
const	char * const *mdoc_argnames = __mdoc_argnames;


const struct mdoc_node *
mdoc_node(const struct mdoc *mdoc)
{

	return(mdoc->first);
}


const struct mdoc_meta *
mdoc_meta(const struct mdoc *mdoc)
{

	return(&mdoc->meta);
}


void
mdoc_free(struct mdoc *mdoc)
{

	if (mdoc->first)
		mdoc_node_freelist(mdoc->first);
	if (mdoc->htab)
		mdoc_tokhash_free(mdoc->htab);
	if (mdoc->meta.title)
		free(mdoc->meta.title);
	if (mdoc->meta.os)
		free(mdoc->meta.os);
	if (mdoc->meta.name)
		free(mdoc->meta.name);
	if (mdoc->meta.arch)
		free(mdoc->meta.arch);
	if (mdoc->meta.vol)
		free(mdoc->meta.vol);

	free(mdoc);
}


struct mdoc *
mdoc_alloc(void *data, const struct mdoc_cb *cb)
{
	struct mdoc	*p;

	p = xcalloc(1, sizeof(struct mdoc));

	p->data = data;
	if (cb)
		(void)memcpy(&p->cb, cb, sizeof(struct mdoc_cb));

	p->last = xcalloc(1, sizeof(struct mdoc_node));
	p->last->type = MDOC_ROOT;
	p->first = p->last;

	p->next = MDOC_NEXT_CHILD;
	p->htab = mdoc_tokhash_alloc();

	return(p);
}


int
mdoc_endparse(struct mdoc *mdoc)
{

	if (MDOC_HALT & mdoc->flags)
		return(0);
	if (NULL == mdoc->first)
		return(1);

	assert(mdoc->last);
	if ( ! macro_end(mdoc)) {
		mdoc->flags |= MDOC_HALT;
		return(0);
	}
	return(1);
}


/*
 * Main line-parsing routine.  If the line is a macro-line (started with
 * a '.' control character), then pass along to the parser, which parses
 * subsequent macros until the end of line.  If normal text, simply
 * append the entire line to the chain.
 */
int
mdoc_parseln(struct mdoc *mdoc, int line, char *buf)
{
	int		  c, i;
	char		  tmp[5];

	if (MDOC_HALT & mdoc->flags)
		return(0);

	mdoc->linetok = 0;

	if ('.' != *buf) {
		/*
		 * Free-form text.  Not allowed in the prologue.
		 */
		if (SEC_PROLOGUE == mdoc->lastnamed)
			return(mdoc_perr(mdoc, line, 0, 
					"no text in prologue"));

		if ( ! mdoc_word_alloc(mdoc, line, 0, buf))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
		return(1);
	}

	/*
	 * Control-character detected.  Begin the parsing sequence.
	 */

	if (buf[1] && '\\' == buf[1])
		if (buf[2] && '\"' == buf[2])
			return(1);

	i = 1;
	while (buf[i] && ! isspace((u_char)buf[i]) && 
			i < (int)sizeof(tmp))
		i++;

	if (i == (int)sizeof(tmp)) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, "unknown macro"));
	} else if (i <= 2) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, "unknown macro"));
	}

	i--;

	(void)memcpy(tmp, buf + 1, (size_t)i);
	tmp[i++] = 0;

	if (MDOC_MAX == (c = mdoc_find(mdoc, tmp))) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, "unknown macro"));
	}

	while (buf[i] && isspace((u_char)buf[i]))
		i++;

	if ( ! mdoc_macro(mdoc, c, line, 1, &i, buf)) {
		mdoc->flags |= MDOC_HALT;
		return(0);
	}

	return(1);
}


void
mdoc_vmsg(struct mdoc *mdoc, int ln, int pos, const char *fmt, ...)
{
	char		  buf[256];
	va_list		  ap;

	if (NULL == mdoc->cb.mdoc_msg)
		return;

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	(*mdoc->cb.mdoc_msg)(mdoc->data, ln, pos, buf);
}


int
mdoc_verr(struct mdoc *mdoc, int ln, int pos, 
		const char *fmt, ...)
{
	char		 buf[256];
	va_list		 ap;

	if (NULL == mdoc->cb.mdoc_err)
		return(0);

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	return((*mdoc->cb.mdoc_err)(mdoc->data, ln, pos, buf));
}


int
mdoc_vwarn(struct mdoc *mdoc, int ln, int pos, 
		enum mdoc_warn type, const char *fmt, ...)
{
	char		 buf[256];
	va_list		 ap;

	if (NULL == mdoc->cb.mdoc_warn)
		return(0);

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	return((*mdoc->cb.mdoc_warn)(mdoc->data, ln, pos, type, buf));
}


int
mdoc_macro(struct mdoc *mdoc, int tok, 
		int ln, int ppos, int *pos, char *buf)
{

	assert(mdoc_macros[tok].fp);

	if (MDOC_PROLOGUE & mdoc_macros[tok].flags && 
			SEC_PROLOGUE != mdoc->lastnamed)
		return(mdoc_perr(mdoc, ln, ppos, "macro disallowed in document body"));
	if ( ! (MDOC_PROLOGUE & mdoc_macros[tok].flags) && 
			SEC_PROLOGUE == mdoc->lastnamed)
		return(mdoc_perr(mdoc, ln, ppos, "macro disallowed in document prologue"));
	if (1 != ppos && ! (MDOC_CALLABLE & mdoc_macros[tok].flags))
		return(mdoc_perr(mdoc, ln, ppos, "macro not callable"));
	return((*mdoc_macros[tok].fp)(mdoc, tok, ln, ppos, pos, buf));
}


static int
mdoc_node_append(struct mdoc *mdoc, struct mdoc_node *p)
{

	assert(mdoc->last);
	assert(mdoc->first);
	assert(MDOC_ROOT != p->type);

	/* See if we exceed the suggest line-max. */

	switch (p->type) {
	case (MDOC_TEXT):
		/* FALLTHROUGH */
	case (MDOC_ELEM):
		/* FALLTHROUGH */
	case (MDOC_BLOCK):
		mdoc->linetok++;
		break;
	default:
		break;
	}

	/* This sort-of works (re-opening of text macros...). */
	if (mdoc->linetok > MDOC_LINEARG_SOFTMAX) 
		if ( ! mdoc_nwarn(mdoc, p, WARN_COMPAT, 
					"suggested %d tokens per line exceeded (has %d)",
					MDOC_LINEARG_SOFTMAX, mdoc->linetok))
			return(0);

	switch (mdoc->next) {
	case (MDOC_NEXT_SIBLING):
		mdoc->last->next = p;
		p->prev = mdoc->last;
		p->parent = mdoc->last->parent;
		break;
	case (MDOC_NEXT_CHILD):
		mdoc->last->child = p;
		p->parent = mdoc->last;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if ( ! mdoc_valid_pre(mdoc, p))
		return(0);

	switch (p->type) {
	case (MDOC_HEAD):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.head = p;
		break;
	case (MDOC_TAIL):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.tail = p;
		break;
	case (MDOC_BODY):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.body = p;
		break;
	default:
		break;
	}

	mdoc->last = p;
	return(1);
}


static struct mdoc_node *
mdoc_node_alloc(const struct mdoc *mdoc)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->sec = mdoc->lastsec;

	return(p);
}


int
mdoc_tail_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_TAIL;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_head_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_HEAD;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_body_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_BODY;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_root_alloc(struct mdoc *mdoc)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->type = MDOC_ROOT;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_block_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->pos = pos;
	p->line = line;
	p->type = MDOC_BLOCK;
	p->tok = tok;
	p->data.block.argc = argsz;
	p->data.block.argv = argdup(argsz, args);

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_elem_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_ELEM;
	p->tok = tok;
	p->data.elem.argc = argsz;
	p->data.elem.argv = argdup(argsz, args);

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_word_alloc(struct mdoc *mdoc, 
		int line, int pos, const char *word)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_TEXT;
	p->data.text.string = xstrdup(word);

	return(mdoc_node_append(mdoc, p));
}


static void
argfree(size_t sz, struct mdoc_arg *p)
{
	int		 i, j;

	if (0 == sz)
		return;

	assert(p);
	/* LINTED */
	for (i = 0; i < (int)sz; i++)
		if (p[i].sz > 0) {
			assert(p[i].value);
			/* LINTED */
			for (j = 0; j < (int)p[i].sz; j++)
				free(p[i].value[j]);
			free(p[i].value);
		}
	free(p);
}


static void
mdoc_elem_free(struct mdoc_elem *p)
{

	argfree(p->argc, p->argv);
}


static void
mdoc_block_free(struct mdoc_block *p)
{

	argfree(p->argc, p->argv);
}


static void
mdoc_text_free(struct mdoc_text *p)
{

	if (p->string)
		free(p->string);
}


void
mdoc_node_free(struct mdoc_node *p)
{

	switch (p->type) {
	case (MDOC_TEXT):
		mdoc_text_free(&p->data.text);
		break;
	case (MDOC_ELEM):
		mdoc_elem_free(&p->data.elem);
		break;
	case (MDOC_BLOCK):
		mdoc_block_free(&p->data.block);
		break;
	default:
		break;
	}

	free(p);
}


void
mdoc_node_freelist(struct mdoc_node *p)
{

	if (p->child)
		mdoc_node_freelist(p->child);
	if (p->next)
		mdoc_node_freelist(p->next);

	mdoc_node_free(p);
}


int
mdoc_find(const struct mdoc *mdoc, const char *key)
{

	return(mdoc_tokhash_find(mdoc->htab, key));
}


static void
argcpy(struct mdoc_arg *dst, const struct mdoc_arg *src)
{
	int		 i;

	dst->line = src->line;
	dst->pos = src->pos;
	dst->arg = src->arg;
	if (0 == (dst->sz = src->sz))
		return;
	dst->value = xcalloc(dst->sz, sizeof(char *));
	for (i = 0; i < (int)dst->sz; i++)
		dst->value[i] = xstrdup(src->value[i]);
}


static struct mdoc_arg *
argdup(size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_arg	*pp;
	int		 i;

	if (0 == argsz)
		return(NULL);

	pp = xcalloc((size_t)argsz, sizeof(struct mdoc_arg));
	for (i = 0; i < (int)argsz; i++)
		argcpy(&pp[i], &args[i]);

	return(pp);
}

