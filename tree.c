/* $Id: tree.c,v 1.5 2009/02/20 23:35:36 kristaps Exp $ */
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
#include <stdlib.h>
#include <stdio.h>

#include "mdoc.h"

#define	xprintf		(void)printf

static	void	 treeprint_r(const struct mdoc_node *, int);

static void
treeprint_r(const struct mdoc_node *n, int indent)
{
	const char	 *p, *t;
	int		  i, j;
	size_t		  argc, sz;
	char		**params;
	struct mdoc_arg	 *argv;

	argv = NULL;
	argc = sz = 0;
	params = NULL;

	switch (n->type) {
	case (MDOC_ROOT):
		t = "root";
		break;
	case (MDOC_BLOCK):
		t = "block";
		break;
	case (MDOC_HEAD):
		t = "block-head";
		break;
	case (MDOC_BODY):
		t = "block-body";
		break;
	case (MDOC_TAIL):
		t = "block-tail";
		break;
	case (MDOC_ELEM):
		t = "elem";
		break;
	case (MDOC_TEXT):
		t = "text";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	switch (n->type) {
	case (MDOC_TEXT):
		p = n->data.text.string;
		break;
	case (MDOC_BODY):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_HEAD):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_TAIL):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_ELEM):
		p = mdoc_macronames[n->tok];
		argv = n->data.elem.argv;
		argc = n->data.elem.argc;
		break;
	case (MDOC_BLOCK):
		p = mdoc_macronames[n->tok];
		argv = n->data.block.argv;
		argc = n->data.block.argc;
		break;
	case (MDOC_ROOT):
		p = "root";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	for (i = 0; i < indent; i++)
		xprintf("    ");
	xprintf("%s (%s)", p, t);

	for (i = 0; i < (int)argc; i++) {
		xprintf(" -%s", mdoc_argnames[argv[i].arg]);
		if (argv[i].sz > 0)
			xprintf(" [");
		for (j = 0; j < (int)argv[i].sz; j++)
			xprintf(" [%s]", argv[i].value[j]);
		if (argv[i].sz > 0)
			xprintf(" ]");
	}

	for (i = 0; i < (int)sz; i++)
		xprintf(" [%s]", params[i]);

	xprintf(" %d:%d\n", n->line, n->pos);

	if (n->child)
		treeprint_r(n->child, indent + 1);
	if (n->next)
		treeprint_r(n->next, indent);
}


/* ARGSUSED */
void
treeprint(const struct mdoc_node *node,
		const struct mdoc_meta *meta)
{

	treeprint_r(node, 0);
}
