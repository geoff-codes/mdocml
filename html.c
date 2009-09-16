/*	$Id: tree.c,v 1.14 2009/08/13 11:45:29 kristaps Exp $ */
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "mdoc.h"
#include "man.h"

#define	DOCTYPE		"-//W3C//DTD HTML 4.01//EN"
#define	DTD		"http://www.w3.org/TR/html4/strict.dtd"

enum	htmltag {
	TAG_HTML,
	TAG_HEAD,
	TAG_BODY,
	TAG_META,
	TAG_TITLE,
	TAG_DIV,
	TAG_H1,
	TAG_H2,
	TAG_P,
	TAG_SPAN,
	TAG_LINK,
	TAG_MAX
};

enum	htmlattr {
	ATTR_HTTPEQUIV,
	ATTR_CONTENT,
	ATTR_NAME,
	ATTR_REL,
	ATTR_HREF,
	ATTR_TYPE,
	ATTR_MEDIA,
	ATTR_CLASS,
	ATTR_MAX
};

struct	htmldata {
	char		 *name;
	int		  flags;
#define	HTML_BLOCK	 (1 << 0)
};

static	const struct htmldata htmltags[TAG_MAX] = {
	{"html",	HTML_BLOCK}, /* TAG_HTML */
	{"head",	HTML_BLOCK}, /* TAG_HEAD */
	{"body",	HTML_BLOCK}, /* TAG_BODY */
	{"meta",	HTML_BLOCK}, /* TAG_META */
	{"title",	HTML_BLOCK}, /* TAG_TITLE */
	{"div",		HTML_BLOCK}, /* TAG_DIV */
	{"h1",		0}, /* TAG_H1 */
	{"h2",		0}, /* TAG_H2 */
	{"p",		HTML_BLOCK}, /* TAG_P */
	{"span",	0}, /* TAG_SPAN */
	{"link",	HTML_BLOCK}, /* TAG_LINK */
};

static	const char	 *const htmlattrs[ATTR_MAX] = {
	"http-equiv",
	"content",
	"name",
	"rel",
	"href",
	"type",
	"media",
	"class"
};

struct	htmlpair {
	enum htmlattr	  key;
	char		 *val;
};

struct	html {
	int		  flags;
#define	HTML_NOSPACE	 (1 << 0)
};

#define	MDOC_ARGS	  const struct mdoc_meta *m, \
			  const struct mdoc_node *n, \
			  struct html *h
#define	MAN_ARGS	  const struct man_meta *m, \
			  const struct man_node *n, \
			  struct html *h
struct	htmlmdoc {
	int		(*pre)(MDOC_ARGS);
	void		(*post)(MDOC_ARGS);
};

static	void		  print_gen_doctype(struct html *);
static	void		  print_gen_head(struct html *);
static	void		  print_mdoc(MDOC_ARGS);
static	void		  print_mdoc_head(MDOC_ARGS);
static	void		  print_mdoc_node(MDOC_ARGS);
static	void		  print_man(MAN_ARGS);
static	void		  print_man_head(MAN_ARGS);
static	void		  print_man_body(MAN_ARGS);
static	void		  print_otag(struct html *, enum htmltag, 
				int, const struct htmlpair *);
static	void		  print_ctag(struct html *, enum htmltag);
static	void		  print_encode(const char *);
static	void		  print_text(struct html *, const char *);
static	int		  mdoc_root_pre(MDOC_ARGS);
static	void		  mdoc_root_post(MDOC_ARGS);

static	int		  mdoc_nd_pre(MDOC_ARGS);
static	int		  mdoc_nm_pre(MDOC_ARGS);
static	void		  mdoc_nm_post(MDOC_ARGS);
static	int		  mdoc_pp_pre(MDOC_ARGS);
static	int		  mdoc_sh_pre(MDOC_ARGS);
static	void		  mdoc_sh_post(MDOC_ARGS);
static	int		  mdoc_ss_pre(MDOC_ARGS);
static	void		  mdoc_ss_post(MDOC_ARGS);

static	const struct htmlmdoc mdocs[MDOC_MAX] = {
	{NULL, NULL}, /* Ap */
	{NULL, NULL}, /* Dd */
	{NULL, NULL}, /* Dt */
	{NULL, NULL}, /* Os */
	{mdoc_sh_pre, mdoc_sh_post }, /* Sh */
	{mdoc_ss_pre, mdoc_ss_post }, /* Ss */ 
	{mdoc_pp_pre, NULL}, /* Pp */ 
	{NULL, NULL}, /* D1 */
	{NULL, NULL}, /* Dl */
	{NULL, NULL}, /* Bd */
	{NULL, NULL}, /* Ed */
	{NULL, NULL}, /* Bl */
	{NULL, NULL}, /* El */
	{NULL, NULL}, /* It */
	{NULL, NULL}, /* Ad */ 
	{NULL, NULL}, /* An */
	{NULL, NULL}, /* Ar */
	{NULL, NULL}, /* Cd */
	{NULL, NULL}, /* Cm */
	{NULL, NULL}, /* Dv */ 
	{NULL, NULL}, /* Er */ 
	{NULL, NULL}, /* Ev */ 
	{NULL, NULL}, /* Ex */
	{NULL, NULL}, /* Fa */ 
	{NULL, NULL}, /* Fd */ 
	{NULL, NULL}, /* Fl */
	{NULL, NULL}, /* Fn */ 
	{NULL, NULL}, /* Ft */ 
	{NULL, NULL}, /* Ic */ 
	{NULL, NULL}, /* In */ 
	{NULL, NULL}, /* Li */
	{mdoc_nd_pre, NULL}, /* Nd */ 
	{mdoc_nm_pre, mdoc_nm_post}, /* Nm */ 
	{NULL, NULL}, /* Op */
	{NULL, NULL}, /* Ot */
	{NULL, NULL}, /* Pa */
	{NULL, NULL}, /* Rv */
	{NULL, NULL}, /* St */ 
	{NULL, NULL}, /* Va */
	{NULL, NULL}, /* Vt */ 
	{NULL, NULL}, /* Xr */
	{NULL, NULL}, /* %A */
	{NULL, NULL}, /* %B */
	{NULL, NULL}, /* %D */
	{NULL, NULL}, /* %I */
	{NULL, NULL}, /* %J */
	{NULL, NULL}, /* %N */
	{NULL, NULL}, /* %O */
	{NULL, NULL}, /* %P */
	{NULL, NULL}, /* %R */
	{NULL, NULL}, /* %T */
	{NULL, NULL}, /* %V */
	{NULL, NULL}, /* Ac */
	{NULL, NULL}, /* Ao */
	{NULL, NULL}, /* Aq */
	{NULL, NULL}, /* At */
	{NULL, NULL}, /* Bc */
	{NULL, NULL}, /* Bf */ 
	{NULL, NULL}, /* Bo */
	{NULL, NULL}, /* Bq */
	{NULL, NULL}, /* Bsx */
	{NULL, NULL}, /* Bx */
	{NULL, NULL}, /* Db */
	{NULL, NULL}, /* Dc */
	{NULL, NULL}, /* Do */
	{NULL, NULL}, /* Dq */
	{NULL, NULL}, /* Ec */
	{NULL, NULL}, /* Ef */
	{NULL, NULL}, /* Em */ 
	{NULL, NULL}, /* Eo */
	{NULL, NULL}, /* Fx */
	{NULL, NULL}, /* Ms */
	{NULL, NULL}, /* No */
	{NULL, NULL}, /* Ns */
	{NULL, NULL}, /* Nx */
	{NULL, NULL}, /* Ox */
	{NULL, NULL}, /* Pc */
	{NULL, NULL}, /* Pf */
	{NULL, NULL}, /* Po */
	{NULL, NULL}, /* Pq */
	{NULL, NULL}, /* Qc */
	{NULL, NULL}, /* Ql */
	{NULL, NULL}, /* Qo */
	{NULL, NULL}, /* Qq */
	{NULL, NULL}, /* Re */
	{NULL, NULL}, /* Rs */
	{NULL, NULL}, /* Sc */
	{NULL, NULL}, /* So */
	{NULL, NULL}, /* Sq */
	{NULL, NULL}, /* Sm */
	{NULL, NULL}, /* Sx */
	{NULL, NULL}, /* Sy */
	{NULL, NULL}, /* Tn */
	{NULL, NULL}, /* Ux */
	{NULL, NULL}, /* Xc */
	{NULL, NULL}, /* Xo */
	{NULL, NULL}, /* Fo */ 
	{NULL, NULL}, /* Fc */ 
	{NULL, NULL}, /* Oo */
	{NULL, NULL}, /* Oc */
	{NULL, NULL}, /* Bk */
	{NULL, NULL}, /* Ek */
	{NULL, NULL}, /* Bt */
	{NULL, NULL}, /* Hf */
	{NULL, NULL}, /* Fr */
	{NULL, NULL}, /* Ud */
	{NULL, NULL}, /* Lb */
	{NULL, NULL}, /* Lp */ 
	{NULL, NULL}, /* Lk */ 
	{NULL, NULL}, /* Mt */ 
	{NULL, NULL}, /* Brq */ 
	{NULL, NULL}, /* Bro */ 
	{NULL, NULL}, /* Brc */ 
	{NULL, NULL}, /* %C */ 
	{NULL, NULL}, /* Es */ 
	{NULL, NULL}, /* En */ 
	{NULL, NULL}, /* Dx */ 
	{NULL, NULL}, /* %Q */ 
	{NULL, NULL}, /* br */
	{NULL, NULL}, /* sp */ 
};


int
html_mdoc(void *arg, const struct mdoc *m)
{
	struct html 	*h;

	h = (struct html *)arg;

	print_gen_doctype(h);
	print_otag(h, TAG_HTML, 0, NULL);
	print_mdoc(mdoc_meta(m), mdoc_node(m), h);
	print_ctag(h, TAG_HTML);
	printf("\n");
	return(1);
}


int
html_man(void *arg, const struct man *m)
{
	struct html	*h;

	h = (struct html *)arg;

	print_gen_doctype(h);
	print_otag(h, TAG_HTML, 0, NULL);
	print_man(man_meta(m), man_node(m), h);
	print_ctag(h, TAG_HTML);
	printf("\n");
	return(1);
}


void *
html_alloc(void)
{

	return(calloc(1, sizeof(struct html)));
}


void
html_free(void *p)
{

	free(p);
}


static void
print_mdoc(MDOC_ARGS)
{

	print_otag(h, TAG_HEAD, 0, NULL);
	print_mdoc_head(m, n, h);
	print_ctag(h, TAG_HEAD);
	print_otag(h, TAG_BODY, 0, NULL);
	print_mdoc_node(m, n, h);
	print_ctag(h, TAG_BODY);
}


static void
print_gen_head(struct html *h)
{
	struct htmlpair	 meta0[2];
	struct htmlpair	 meta1[2];
	struct htmlpair	 link[4];

	meta0[0].key = ATTR_HTTPEQUIV;
	meta0[0].val = "Content-Type";
	meta0[1].key = ATTR_CONTENT;
	meta0[1].val = "text/html; charest-utf-8";

	meta1[0].key = ATTR_NAME;
	meta1[0].val = "resource-type";
	meta1[1].key = ATTR_CONTENT;
	meta1[1].val = "document";

	link[0].key = ATTR_REL;
	link[0].val = "stylesheet";
	link[1].key = ATTR_HREF;
	link[1].val = "style.css";
	link[2].key = ATTR_TYPE;
	link[2].val = "text/css";
	link[3].key = ATTR_MEDIA;
	link[3].val = "all";

	print_otag(h, TAG_META, 2, meta0);
	print_otag(h, TAG_META, 2, meta1);
	print_otag(h, TAG_LINK, 4, link);
}


static void
print_mdoc_head(MDOC_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(m->title);
	print_ctag(h, TAG_TITLE);
}


static int
mdoc_root_pre(MDOC_ARGS)
{
	struct htmlpair	 div;

	div.key = ATTR_CLASS;
	div.val = "body";

	print_otag(h, TAG_DIV, 1, &div);
	return(1);
}


static void
mdoc_root_post(MDOC_ARGS)
{

	print_ctag(h, TAG_DIV);
}


static int
mdoc_ss_pre(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_otag(h, TAG_P, 0, NULL);
	if (MDOC_HEAD == n->type)
		print_otag(h, TAG_H2, 0, NULL);
	return(1);
}


static void
mdoc_ss_post(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_ctag(h, TAG_P);
	if (MDOC_HEAD == n->type)
		print_ctag(h, TAG_H2);
}


static int
mdoc_pp_pre(MDOC_ARGS)
{

	print_otag(h, TAG_P, 0, NULL);
	return(0);
}


static int
mdoc_nd_pre(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_text(h, "--");
	return(1);
}


static int
mdoc_nm_pre(MDOC_ARGS)
{
	struct htmlpair	class;

	class.key = ATTR_CLASS;
	class.val = "name";

	print_otag(h, TAG_SPAN, 1, &class);
	if (NULL == n->child)
		print_text(h, m->name);

	return(1);
}


static void
mdoc_nm_post(MDOC_ARGS)
{

	print_ctag(h, TAG_SPAN);
}


static int
mdoc_sh_pre(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_otag(h, TAG_P, 0, NULL);
	if (MDOC_HEAD == n->type)
		print_otag(h, TAG_H1, 0, NULL);
	return(1);
}


static void
mdoc_sh_post(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_ctag(h, TAG_P);
	if (MDOC_HEAD == n->type)
		print_ctag(h, TAG_H1);
}


static void
print_mdoc_node(MDOC_ARGS)
{
	int		 child;

	child = 1;

	switch (n->type) {
	case (MDOC_ROOT):
		child = mdoc_root_pre(m, n, h);
		break;
	case (MDOC_TEXT):
		print_text(h, n->string);
		break;
	default:
		if (mdocs[n->tok].pre)
			child = (*mdocs[n->tok].pre)(m, n, h);
		break;
	}

	if (child && n->child)
		print_mdoc_node(m, n->child, h);

	switch (n->type) {
	case (MDOC_ROOT):
		mdoc_root_post(m, n, h);
		break;
	case (MDOC_TEXT):
		break;
	default:
		if (mdocs[n->tok].post)
			(*mdocs[n->tok].post)(m, n, h);
		break;
	}

	if (n->next)
		print_mdoc_node(m, n->next, h);
}


static void
print_man(MAN_ARGS)
{

	print_otag(h, TAG_HEAD, 0, NULL);
	print_man_head(m, n, h);
	print_ctag(h, TAG_HEAD);
	print_otag(h, TAG_BODY, 0, NULL);
	print_man_body(m, n, h);
	print_ctag(h, TAG_BODY);
}


static void
print_man_head(MAN_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(m->title);
	print_ctag(h, TAG_TITLE);
}


static void
print_man_body(MAN_ARGS)
{
}


static void
print_encode(const char *p)
{

	printf("%s", p); /* XXX */
}


static void
print_otag(struct html *h, enum htmltag tag, 
		int sz, const struct htmlpair *p)
{
	int		 i;

	if ( ! (HTML_NOSPACE & h->flags))
		if ( ! (HTML_BLOCK & htmltags[tag].flags))
			printf(" ");

	printf("<%s", htmltags[tag].name);
	for (i = 0; i < sz; i++) {
		printf(" %s=\"", htmlattrs[p[i].key]);
		assert(p->val);
		print_encode(p[i].val);
		printf("\"");
	}
	printf(">");

	h->flags |= HTML_NOSPACE;

}


/* ARGSUSED */
static void
print_ctag(struct html *h, enum htmltag tag)
{
	
	printf("</%s>", htmltags[tag].name);
	if (HTML_BLOCK & htmltags[tag].flags)
		h->flags |= HTML_NOSPACE;
}


/* ARGSUSED */
static void
print_gen_doctype(struct html *h)
{
	
	printf("<!DOCTYPE HTML PUBLIC \"%s\" \"%s\">\n", DOCTYPE, DTD);
}


static void
print_text(struct html *h, const char *p)
{

	if (*p && 0 == *(p + 1))
		switch (*p) {
		case('.'):
			/* FALLTHROUGH */
		case(','):
			/* FALLTHROUGH */
		case(';'):
			/* FALLTHROUGH */
		case(':'):
			/* FALLTHROUGH */
		case('?'):
			/* FALLTHROUGH */
		case('!'):
			/* FALLTHROUGH */
		case(')'):
			/* FALLTHROUGH */
		case(']'):
			/* FALLTHROUGH */
		case('}'):
			h->flags |= HTML_NOSPACE;
		default:
			break;
		}

	if ( ! (h->flags & HTML_NOSPACE))
		printf(" ");
	h->flags &= ~HTML_NOSPACE;

	if (p)
		print_encode(p);

	if (*p && 0 == *(p + 1))
		switch (*p) {
		case('('):
			/* FALLTHROUGH */
		case('['):
			/* FALLTHROUGH */
		case('{'):
			h->flags |= HTML_NOSPACE;
		default:
			break;
		}
}
