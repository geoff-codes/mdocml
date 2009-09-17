/*	$Id: html.c,v 1.33 2009/09/17 13:17:30 kristaps Exp $ */
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
#include <sys/queue.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chars.h"
#include "mdoc.h"
#include "man.h"

#define	DOCTYPE		"-//W3C//DTD HTML 4.01//EN"
#define	DTD		"http://www.w3.org/TR/html4/strict.dtd"

#define	INDENT		 5
#define	HALFINDENT	 3
#define	PX_MULT		 8

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
	TAG_BR,
	TAG_A,
	TAG_TABLE,
	TAG_COL,
	TAG_TR,
	TAG_TD,
	TAG_LI,
	TAG_UL,
	TAG_OL,
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
	ATTR_STYLE,
	ATTR_WIDTH,
	ATTR_VALIGN,
	ATTR_MAX
};

struct	htmldata {
	char		 *name;
	int		  flags;
#define	HTML_CLRLINE	 (1 << 0)
#define	HTML_NOSTACK	 (1 << 1)
};

static	const struct htmldata htmltags[TAG_MAX] = {
	{"html",	HTML_CLRLINE}, /* TAG_HTML */
	{"head",	HTML_CLRLINE}, /* TAG_HEAD */
	{"body",	HTML_CLRLINE}, /* TAG_BODY */
	{"meta",	HTML_CLRLINE | HTML_NOSTACK}, /* TAG_META */
	{"title",	HTML_CLRLINE}, /* TAG_TITLE */
	{"div",		HTML_CLRLINE}, /* TAG_DIV */
	{"h1",		0}, /* TAG_H1 */
	{"h2",		0}, /* TAG_H2 */
	{"p",		HTML_CLRLINE}, /* TAG_P */
	{"span",	0}, /* TAG_SPAN */
	{"link",	HTML_CLRLINE | HTML_NOSTACK}, /* TAG_LINK */
	{"br",		HTML_CLRLINE | HTML_NOSTACK}, /* TAG_LINK */
	{"a",		0}, /* TAG_A */
	{"table",	HTML_CLRLINE}, /* TAG_TABLE */
	{"col",		HTML_CLRLINE | HTML_NOSTACK}, /* TAG_COL */
	{"tr",		HTML_CLRLINE}, /* TAG_TR */
	{"td",		HTML_CLRLINE}, /* TAG_TD */
	{"li",		HTML_CLRLINE}, /* TAG_LI */
	{"ul",		HTML_CLRLINE}, /* TAG_UL */
	{"ol",		HTML_CLRLINE}, /* TAG_OL */
};

static	const char	 *const htmlattrs[ATTR_MAX] = {
	"http-equiv",
	"content",
	"name",
	"rel",
	"href",
	"type",
	"media",
	"class",
	"style",
	"width",
	"valign",
};

struct	htmlpair {
	enum htmlattr	  key;
	char		 *val;
};

struct	tag {
	enum htmltag	  tag;
	SLIST_ENTRY(tag)  entry;
};

SLIST_HEAD(tagq, tag);

struct	html {
	int		  flags;
#define	HTML_NOSPACE	 (1 << 0)
#define	HTML_NEWLINE	 (1 << 1)
	struct tagq	  stack;
	void		 *symtab;
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
static	void		  print_mdoc_title(MDOC_ARGS);
static	void		  print_mdoc_node(MDOC_ARGS);
static	void		  print_man(MAN_ARGS);
static	void		  print_man_head(MAN_ARGS);
static	void		  print_man_body(MAN_ARGS);
static	struct tag	 *print_otag(struct html *, enum htmltag, 
				int, const struct htmlpair *);
static	void		  print_tagq(struct html *, const struct tag *);
static	void		  print_stagq(struct html *, const struct tag *);
static	void		  print_ctag(struct html *, enum htmltag);
static	void		  print_encode(struct html *, const char *);
static	void		  print_escape(struct html *, const char **);
static	void		  print_text(struct html *, const char *);
static	void		  print_res(struct html *, const char *, int);
static	void		  print_spec(struct html *, const char *, int);

static	int		  a2width(const char *);
static	int		  a2offs(const char *);

static	int		  mdoc_list_pre(MDOC_ARGS, int);
static	int		  mdoc_listitem_pre(MDOC_ARGS);
static	int		  mdoc_root_pre(MDOC_ARGS);
static	int		  mdoc_tbl_pre(MDOC_ARGS, int);
static	int		  mdoc_tbl_block_pre(MDOC_ARGS, int, int, int);
static	int		  mdoc_tbl_body_pre(MDOC_ARGS, int, int);
static	int		  mdoc_tbl_head_pre(MDOC_ARGS, int, int);

static	int		  mdoc_ar_pre(MDOC_ARGS);
static	int		  mdoc_bl_pre(MDOC_ARGS);
static	int		  mdoc_d1_pre(MDOC_ARGS);
static	void		  mdoc_dq_post(MDOC_ARGS);
static	int		  mdoc_dq_pre(MDOC_ARGS);
static	int		  mdoc_fl_pre(MDOC_ARGS);
static	int		  mdoc_em_pre(MDOC_ARGS);
static	int		  mdoc_ex_pre(MDOC_ARGS);
static	int		  mdoc_it_pre(MDOC_ARGS);
static	int		  mdoc_nd_pre(MDOC_ARGS);
static	int		  mdoc_nm_pre(MDOC_ARGS);
static	int		  mdoc_ns_pre(MDOC_ARGS);
static	void		  mdoc_op_post(MDOC_ARGS);
static	int		  mdoc_op_pre(MDOC_ARGS);
static	int		  mdoc_pp_pre(MDOC_ARGS);
static	void		  mdoc_pq_post(MDOC_ARGS);
static	int		  mdoc_pq_pre(MDOC_ARGS);
static	int		  mdoc_sh_pre(MDOC_ARGS);
static	void		  mdoc_sq_post(MDOC_ARGS);
static	int		  mdoc_sq_pre(MDOC_ARGS);
static	int		  mdoc_ss_pre(MDOC_ARGS);
static	int		  mdoc_sx_pre(MDOC_ARGS);
static	int		  mdoc_xr_pre(MDOC_ARGS);
static	int		  mdoc_xx_pre(MDOC_ARGS);

#ifdef __linux__
extern	size_t	  strlcpy(char *, const char *, size_t);
extern	size_t	  strlcat(char *, const char *, size_t);
#endif

static	const struct htmlmdoc mdocs[MDOC_MAX] = {
	{NULL, NULL}, /* Ap */
	{NULL, NULL}, /* Dd */
	{NULL, NULL}, /* Dt */
	{NULL, NULL}, /* Os */
	{mdoc_sh_pre, NULL }, /* Sh */
	{mdoc_ss_pre, NULL }, /* Ss */ 
	{mdoc_pp_pre, NULL}, /* Pp */ 
	{mdoc_d1_pre, NULL}, /* D1 */
	{mdoc_d1_pre, NULL}, /* Dl */
	{NULL, NULL}, /* Bd */
	{NULL, NULL}, /* Ed */
	{mdoc_bl_pre, NULL}, /* Bl */
	{NULL, NULL}, /* El */
	{mdoc_it_pre, NULL}, /* It */
	{NULL, NULL}, /* Ad */ 
	{NULL, NULL}, /* An */
	{mdoc_ar_pre, NULL}, /* Ar */
	{NULL, NULL}, /* Cd */
	{NULL, NULL}, /* Cm */
	{NULL, NULL}, /* Dv */ 
	{NULL, NULL}, /* Er */ 
	{NULL, NULL}, /* Ev */ 
	{mdoc_ex_pre, NULL}, /* Ex */
	{NULL, NULL}, /* Fa */ 
	{NULL, NULL}, /* Fd */ 
	{mdoc_fl_pre, NULL}, /* Fl */
	{NULL, NULL}, /* Fn */ 
	{NULL, NULL}, /* Ft */ 
	{NULL, NULL}, /* Ic */ 
	{NULL, NULL}, /* In */ 
	{NULL, NULL}, /* Li */
	{mdoc_nd_pre, NULL}, /* Nd */ 
	{mdoc_nm_pre, NULL}, /* Nm */ 
	{mdoc_op_pre, mdoc_op_post}, /* Op */
	{NULL, NULL}, /* Ot */
	{NULL, NULL}, /* Pa */
	{NULL, NULL}, /* Rv */
	{NULL, NULL}, /* St */ 
	{NULL, NULL}, /* Va */
	{NULL, NULL}, /* Vt */ 
	{mdoc_xr_pre, NULL}, /* Xr */
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
	{mdoc_xx_pre, NULL}, /* Bsx */
	{NULL, NULL}, /* Bx */
	{NULL, NULL}, /* Db */
	{NULL, NULL}, /* Dc */
	{NULL, NULL}, /* Do */
	{mdoc_dq_pre, mdoc_dq_post}, /* Dq */
	{NULL, NULL}, /* Ec */
	{NULL, NULL}, /* Ef */
	{mdoc_em_pre, NULL}, /* Em */ 
	{NULL, NULL}, /* Eo */
	{mdoc_xx_pre, NULL}, /* Fx */
	{NULL, NULL}, /* Ms */
	{NULL, NULL}, /* No */
	{mdoc_ns_pre, NULL}, /* Ns */
	{mdoc_xx_pre, NULL}, /* Nx */
	{mdoc_xx_pre, NULL}, /* Ox */
	{NULL, NULL}, /* Pc */
	{NULL, NULL}, /* Pf */
	{mdoc_pq_pre, mdoc_pq_post}, /* Po */
	{mdoc_pq_pre, mdoc_pq_post}, /* Pq */
	{NULL, NULL}, /* Qc */
	{NULL, NULL}, /* Ql */
	{NULL, NULL}, /* Qo */
	{NULL, NULL}, /* Qq */
	{NULL, NULL}, /* Re */
	{NULL, NULL}, /* Rs */
	{NULL, NULL}, /* Sc */
	{mdoc_sq_pre, mdoc_sq_post}, /* So */
	{mdoc_sq_pre, mdoc_sq_post}, /* Sq */
	{NULL, NULL}, /* Sm */
	{mdoc_sx_pre, NULL}, /* Sx */
	{NULL, NULL}, /* Sy */
	{NULL, NULL}, /* Tn */
	{mdoc_xx_pre, NULL}, /* Ux */
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
	{mdoc_xx_pre, NULL}, /* Dx */ 
	{NULL, NULL}, /* %Q */ 
	{NULL, NULL}, /* br */
	{NULL, NULL}, /* sp */ 
};


void
html_mdoc(void *arg, const struct mdoc *m)
{
	struct html 	*h;
	struct tag	*t;

	h = (struct html *)arg;

	print_gen_doctype(h);
	t = print_otag(h, TAG_HTML, 0, NULL);
	print_mdoc(mdoc_meta(m), mdoc_node(m), h);
	print_tagq(h, t);

	printf("\n");
}


void
html_man(void *arg, const struct man *m)
{
	struct html	*h;
	struct tag	*t;

	h = (struct html *)arg;

	print_gen_doctype(h);
	t = print_otag(h, TAG_HTML, 0, NULL);
	print_man(man_meta(m), man_node(m), h);
	print_tagq(h, t);

	printf("\n");
}


void *
html_alloc(void)
{
	struct html	*h;

	if (NULL == (h = calloc(1, sizeof(struct html))))
		return(NULL);

	SLIST_INIT(&h->stack);
	if (NULL == (h->symtab = chars_init(CHARS_HTML))) {
		free(h);
		return(NULL);
	}
	return(h);
}


void
html_free(void *p)
{
	struct tag	*tag;
	struct html	*h;

	h = (struct html *)p;

	while ( ! SLIST_EMPTY(&h->stack)) {
		tag = SLIST_FIRST(&h->stack);
		SLIST_REMOVE_HEAD(&h->stack, entry);
		free(tag);
	}
	free(h);
}


static void
print_mdoc(MDOC_ARGS)
{
	struct tag	*t;

	t = print_otag(h, TAG_HEAD, 0, NULL);
	print_mdoc_head(m, n, h);
	print_tagq(h, t);

	t = print_otag(h, TAG_BODY, 0, NULL);
	print_mdoc_title(m, n, h);
	print_mdoc_node(m, n, h);
	print_tagq(h, t);
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
	meta0[1].val = "text/html; charset=utf-8";

	meta1[0].key = ATTR_NAME;
	meta1[0].val = "resource-type";
	meta1[1].key = ATTR_CONTENT;
	meta1[1].val = "document";

	link[0].key = ATTR_REL;
	link[0].val = "stylesheet";
	link[1].key = ATTR_HREF;
	link[1].val = "style.css"; /* XXX */
	link[2].key = ATTR_TYPE;
	link[2].val = "text/css";
	link[3].key = ATTR_MEDIA;
	link[3].val = "all";

	print_otag(h, TAG_META, 2, meta0);
	print_otag(h, TAG_META, 2, meta1);
	print_otag(h, TAG_LINK, 4, link);
}


/* ARGSUSED */
static void
print_mdoc_head(MDOC_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(h, m->title);
}


/* ARGSUSED */
static void
print_mdoc_title(MDOC_ARGS)
{

	/* TODO */
}


static void
print_mdoc_node(MDOC_ARGS)
{
	int		 child;
	struct tag	*t;

	child = 1;
	t = SLIST_FIRST(&h->stack);

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

	print_stagq(h, t);

	switch (n->type) {
	case (MDOC_ROOT):
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
	struct tag	*t;

	t = print_otag(h, TAG_HEAD, 0, NULL);
	print_man_head(m, n, h);
	print_tagq(h, t);

	t = print_otag(h, TAG_BODY, 0, NULL);
	print_man_body(m, n, h);
	print_tagq(h, t);
}


/* ARGSUSED */
static void
print_man_head(MAN_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(h, m->title);
}


/* ARGSUSED */
static void
print_man_body(MAN_ARGS)
{

	/* TODO */
}


static void
print_spec(struct html *h, const char *p, int len)
{
	const char	*rhs;
	int		 i;
	size_t		 sz;

	rhs = chars_a2ascii(h->symtab, p, (size_t)len, &sz);

	if (NULL == rhs) 
		return;
	for (i = 0; i < (int)sz; i++) 
		putchar(rhs[i]);
}


static void
print_res(struct html *h, const char *p, int len)
{
	const char	*rhs;
	int		 i;
	size_t		 sz;

	rhs = chars_a2res(h->symtab, p, (size_t)len, &sz);

	if (NULL == rhs)
		return;
	for (i = 0; i < (int)sz; i++) 
		putchar(rhs[i]);
}


static void
print_escape(struct html *h, const char **p)
{
	int		 j, type;
	const char	*wp;

	wp = *p;
	type = 1;

	if (0 == *(++wp)) {
		*p = wp;
		return;
	}

	if ('(' == *wp) {
		wp++;
		if (0 == *wp || 0 == *(wp + 1)) {
			*p = 0 == *wp ? wp : wp + 1;
			return;
		}

		print_spec(h, wp, 2);
		*p = ++wp;
		return;

	} else if ('*' == *wp) {
		if (0 == *(++wp)) {
			*p = wp;
			return;
		}

		switch (*wp) {
		case ('('):
			wp++;
			if (0 == *wp || 0 == *(wp + 1)) {
				*p = 0 == *wp ? wp : wp + 1;
				return;
			}

			print_res(h, wp, 2);
			*p = ++wp;
			return;
		case ('['):
			type = 0;
			break;
		default:
			print_res(h, wp, 1);
			*p = wp;
			return;
		}
	
	} else if ('f' == *wp) {
		if (0 == *(++wp)) {
			*p = wp;
			return;
		}

		switch (*wp) {
		case ('B'):
			/* TODO */
			break;
		case ('I'):
			/* TODO */
			break;
		case ('P'):
			/* FALLTHROUGH */
		case ('R'):
			/* TODO */
			break;
		default:
			break;
		}

		*p = wp;
		return;

	} else if ('[' != *wp) {
		print_spec(h, wp, 1);
		*p = wp;
		return;
	}

	wp++;
	for (j = 0; *wp && ']' != *wp; wp++, j++)
		/* Loop... */ ;

	if (0 == *wp) {
		*p = wp;
		return;
	}

	if (type)
		print_spec(h, wp - j, j);
	else
		print_res(h, wp - j, j);

	*p = wp;
}


static void
print_encode(struct html *h, const char *p)
{

	for (; *p; p++) {
		if ('\\' == *p) {
			print_escape(h, &p);
			continue;
		}
		switch (*p) {
		case ('<'):
			printf("&lt;");
			break;
		case ('>'):
			printf("&gt;");
			break;
		case ('&'):
			printf("&amp;");
			break;
		default:
			putchar(*p);
			break;
		}
	}
}


static struct tag *
print_otag(struct html *h, enum htmltag tag, 
		int sz, const struct htmlpair *p)
{
	int		 i;
	struct tag	*t;

	if ( ! (HTML_NOSTACK & htmltags[tag].flags)) {
		if (NULL == (t = malloc(sizeof(struct tag))))
			err(EXIT_FAILURE, "malloc");
		t->tag = tag;
		SLIST_INSERT_HEAD(&h->stack, t, entry);
	} else
		t = NULL;

	if ( ! (HTML_NOSPACE & h->flags))
		if ( ! (HTML_CLRLINE & htmltags[tag].flags))
			printf(" ");

	printf("<%s", htmltags[tag].name);
	for (i = 0; i < sz; i++) {
		printf(" %s=\"", htmlattrs[p[i].key]);
		assert(p->val);
		print_encode(h, p[i].val);
		printf("\"");
	}
	printf(">");

	h->flags |= HTML_NOSPACE;
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NEWLINE;
	else
		h->flags &= ~HTML_NEWLINE;

	return(t);
}


/* ARGSUSED */
static void
print_ctag(struct html *h, enum htmltag tag)
{
	
	printf("</%s>", htmltags[tag].name);
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NOSPACE;
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NEWLINE;
	else
		h->flags &= ~HTML_NEWLINE;
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
			break;
		default:
			break;
		}

	if ( ! (h->flags & HTML_NOSPACE))
		printf(" ");

	h->flags &= ~HTML_NOSPACE;
	h->flags &= ~HTML_NEWLINE;

	if (p)
		print_encode(h, p);

	if (*p && 0 == *(p + 1))
		switch (*p) {
		case('('):
			/* FALLTHROUGH */
		case('['):
			/* FALLTHROUGH */
		case('{'):
			h->flags |= HTML_NOSPACE;
			break;
		default:
			break;
		}
}


static void
print_tagq(struct html *h, const struct tag *until)
{
	struct tag	*tag;

	while ( ! SLIST_EMPTY(&h->stack)) {
		tag = SLIST_FIRST(&h->stack);
		print_ctag(h, tag->tag);
		SLIST_REMOVE_HEAD(&h->stack, entry);
		free(tag);
		if (until && tag == until)
			return;
	}
}


static void
print_stagq(struct html *h, const struct tag *suntil)
{
	struct tag	*tag;

	while ( ! SLIST_EMPTY(&h->stack)) {
		tag = SLIST_FIRST(&h->stack);
		if (suntil && tag == suntil)
			return;
		print_ctag(h, tag->tag);
		SLIST_REMOVE_HEAD(&h->stack, entry);
		free(tag);
	}
}


static int
a2offs(const char *p)
{
	int		 len, i;

	if (0 == strcmp(p, "left"))
		return(0);
	if (0 == strcmp(p, "indent"))
		return(INDENT + 1);
	if (0 == strcmp(p, "indent-two"))
		return((INDENT + 1) * 2);

	if (0 == (len = (int)strlen(p)))
		return(0);

	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p));

	return(len);
}


static int
a2width(const char *p)
{
	int		 i, len;

	if (0 == (len = (int)strlen(p)))
		return(0);
	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p) + 2);

	return(len + 2);
}




/* ARGSUSED */
static int
mdoc_root_pre(MDOC_ARGS)
{
	struct htmlpair	 tag;

	tag.key = ATTR_CLASS;
	tag.val = "body";

	print_otag(h, TAG_DIV, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_ss_pre(MDOC_ARGS)
{
	struct htmlpair	tag[2];

	tag[0].key = ATTR_CLASS;
	tag[0].val = "ssec";

	tag[1].key = ATTR_STYLE;
	tag[1].val = "margin-left: -20px;";

	if (MDOC_BODY == n->type)
		print_otag(h, TAG_DIV, 1, &tag);
	if (MDOC_HEAD == n->type)
		print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_fl_pre(MDOC_ARGS)
{
	struct htmlpair	 tag;

	tag.key = ATTR_CLASS;
	tag.val = "flag";

	print_otag(h, TAG_SPAN, 1, &tag);
	print_text(h, "\\-");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
mdoc_pp_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_STYLE;
	tag.val = "clear: both;";

	print_otag(h, TAG_BR, 1, &tag);
	print_otag(h, TAG_BR, 1, &tag);
	return(0);
}


/* ARGSUSED */
static int
mdoc_nd_pre(MDOC_ARGS)
{

	if (MDOC_BODY == n->type)
		print_text(h, "\\(en");
	return(1);
}


/* ARGSUSED */
static int
mdoc_op_pre(MDOC_ARGS)
{

	if (MDOC_BODY == n->type) {
		print_text(h, "\\(lB");
		h->flags |= HTML_NOSPACE;
	}
	return(1);
}


/* ARGSUSED */
static void
mdoc_op_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type) 
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(rB");
}


static int
mdoc_nm_pre(MDOC_ARGS)
{
	struct htmlpair	class;

	if ( ! (HTML_NEWLINE & h->flags))
		if (SEC_SYNOPSIS == n->sec)
			print_otag(h, TAG_BR, 0, NULL);

	class.key = ATTR_CLASS;
	class.val = "name";

	print_otag(h, TAG_SPAN, 1, &class);
	if (NULL == n->child)
		print_text(h, m->name);

	return(1);
}


/* ARGSUSED */
static int
mdoc_sh_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "sec";

	if (MDOC_BODY == n->type)
		print_otag(h, TAG_DIV, 1, &tag);
	if (MDOC_HEAD == n->type)
		print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_xr_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_HREF;
	tag.val = "#"; /* TODO */

	print_otag(h, TAG_A, 1, &tag);

	n = n->child;
	print_text(h, n->string);
	if (NULL == (n = n->next))
		return(0);

	h->flags |= HTML_NOSPACE;
	print_text(h, "(");
	h->flags |= HTML_NOSPACE;
	print_text(h, n->string);
	h->flags |= HTML_NOSPACE;
	print_text(h, ")");

	return(0);
}


/* ARGSUSED */
static int
mdoc_ns_pre(MDOC_ARGS)
{

	h->flags |= HTML_NOSPACE;
	return(1);
}

/* ARGSUSED */
static int
mdoc_ar_pre(MDOC_ARGS)
{
	struct htmlpair tag;

	tag.key = ATTR_CLASS;
	tag.val = "arg";

	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}

/* ARGSUSED */
static int
mdoc_xx_pre(MDOC_ARGS)
{
	const char	*pp;

	switch (n->tok) {
	case (MDOC_Bsx):
		pp = "BSDI BSD/OS";
		break;
	case (MDOC_Dx):
		pp = "DragonFlyBSD";
		break;
	case (MDOC_Fx):
		pp = "FreeBSD";
		break;
	case (MDOC_Nx):
		pp = "NetBSD";
		break;
	case (MDOC_Ox):
		pp = "OpenBSD";
		break;
	case (MDOC_Ux):
		pp = "UNIX";
		break;
	default:
		return(1);
	}

	print_text(h, pp);
	return(1);
}


static int
mdoc_tbl_block_pre(MDOC_ARGS, int w, int o, int c)
{
	struct htmlpair	 tag;
	char		 buf[BUFSIZ];

	buf[BUFSIZ - 1] = 0;

	snprintf(buf, BUFSIZ - 1, "margin-left: %dpx; "
			"clear: both;", w + o);

	if ( ! c)
		(void)strlcat(buf, " padding-top: 1em;", BUFSIZ);

	tag.key = ATTR_STYLE;
	tag.val = buf;

	print_otag(h, TAG_DIV, 1, &tag);
	return(1);
}


static int
mdoc_tbl_body_pre(MDOC_ARGS, int t, int w)
{
	struct htmlpair	 tag;
	char		 buf[BUFSIZ];
	int		 i;

	buf[BUFSIZ - 1] = 0;
	i = 0;

	switch (t) {
	case (MDOC_Tag):
		i++;
		(void)snprintf(buf, BUFSIZ - 1, 
				"clear: right; float: left; "
				"width: 100%%;");
		tag.key = ATTR_STYLE;
		tag.val = buf;
		break;
	default:
		break;
	}

	print_otag(h, TAG_DIV, i, &tag);
	return(1);
}


static int
mdoc_tbl_head_pre(MDOC_ARGS, int type, int w)
{
	struct htmlpair	 tag;
	char		 buf[BUFSIZ];
	int		 i;

	buf[BUFSIZ - 1] = 0;
	i = 0;

	switch (type) {
	case (MDOC_Tag):
		i++;
		(void)snprintf(buf, BUFSIZ - 1,  
				"clear: left; float: left; "
				"padding-right: 1em; "
				"margin-left: -%dpx;", w);
		tag.key = ATTR_STYLE;
		tag.val = buf;
		break;
	default:
		i++;
		(void)snprintf(buf, BUFSIZ - 1, 
				"clear: left; float: left; "
				"margin-left: -%dpx; "
				"padding-right: 1em;", w);
		tag.key = ATTR_STYLE;
		tag.val = buf;
		break;
	}

	print_otag(h, TAG_DIV, i, &tag);
	return(1);
}


static int
mdoc_tbl_pre(MDOC_ARGS, int type)
{
	int			 i, w, o, c;
	const struct mdoc_node	*bl;

	bl = n->parent->parent;
	if (MDOC_BLOCK != n->type) 
		bl = bl->parent;

	/* FIXME: fmt_vspace() equivalent. */

	assert(bl->args);

	w = o = c = 0;

	for (i = 0; i < (int)bl->args->argc; i++) 
		if (MDOC_Width == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			w = a2width(bl->args->argv[i].value[0]);
		} else if (MDOC_Offset == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			o = a2offs(bl->args->argv[i].value[0]);
		} else if (MDOC_Compact == bl->args->argv[i].arg) 
			c = 1;

	if (0 == w)
		w = 10;

	w *= PX_MULT;
	o *= PX_MULT;
	
	switch (n->type) {
	case (MDOC_BLOCK):
		break;
	case (MDOC_HEAD):
		return(mdoc_tbl_head_pre(m, n, h, type, w));
	case (MDOC_BODY):
		return(mdoc_tbl_body_pre(m, n, h, type, w));
	default:
		abort();
		/* NOTREACHED */
	}

	return(mdoc_tbl_block_pre(m, n, h, w, o, c));
}


/* ARGSUSED */
static int
mdoc_listitem_pre(MDOC_ARGS)
{
	int			 i, w, o, c;
	const struct mdoc_node	*bl;
	struct htmlpair	 	 tag;
	char		 	 buf[BUFSIZ];

	/* FIXME: fmt_vspace() equivalent. */

	if (MDOC_BLOCK != n->type)
		return(1);

	bl = n->parent->parent;
	assert(bl);

	w = o = c = 0;

	for (i = 0; i < (int)bl->args->argc; i++) 
		if (MDOC_Width == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			w = a2width(bl->args->argv[i].value[0]);
		} else if (MDOC_Offset == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			o = a2offs(bl->args->argv[i].value[0]);
		} else if (MDOC_Compact == bl->args->argv[i].arg) 
			c = 1;
	
	o *= PX_MULT;
	w *= PX_MULT;

	buf[BUFSIZ - 1] = 0;

	snprintf(buf, BUFSIZ - 1, "margin-left: %dpx;", o);

	if ( ! c)
		(void)strlcat(buf, " padding-top: 1em;", BUFSIZ);

	tag.key = ATTR_STYLE;
	tag.val = buf;

	print_otag(h, TAG_LI, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_list_pre(MDOC_ARGS, int type)
{

	switch (type) {
	case (MDOC_Enum):
		print_otag(h, TAG_OL, 0, NULL);
		break;
	case (MDOC_Bullet):
		print_otag(h, TAG_UL, 0, NULL);
		break;
	default:
		break;
	}

	return(1);
}


static int
mdoc_bl_pre(MDOC_ARGS)
{
	int		i, len, type;

	if (MDOC_BLOCK != n->type)
		return(1);

	assert(n->args);
	len = (int)n->args->argc;

	for (i = 0; i < len; i++) 
		switch ((type = n->args->argv[i].arg)) {
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Bullet):
			return(mdoc_list_pre(m, n, h, type));
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Item):
			/* FALLTHROUGH */
		case (MDOC_Column):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			return(1); 
		default:
			break;
		}

	abort();
	/* NOTREACHED */
}


static int
mdoc_it_pre(MDOC_ARGS)
{
	int		 	 i, len, type;
	const struct mdoc_node	*bl;

	if (MDOC_BLOCK == n->type)
		bl = n->parent->parent;
	else
		bl = n->parent->parent->parent;

	assert(bl->args);
	len = (int)bl->args->argc;

	for (i = 0; i < len; i++) 
		switch ((type = bl->args->argv[i].arg)) {
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			return(mdoc_tbl_pre(m, n, h, type));
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Bullet):
			return(mdoc_listitem_pre(m, n, h));
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Item):
			/* FALLTHROUGH */
		case (MDOC_Column):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			return(0);
		default:
			break;
		}

	abort();
	/* NOTREACHED */
}


/* ARGSUSED */
static int
mdoc_ex_pre(MDOC_ARGS)
{
	const struct mdoc_node	*nn;
	struct tag		*t;
	struct htmlpair		 tag;

	print_text(h, "The");

	tag.key = ATTR_CLASS;
	tag.val = "utility";

	for (nn = n->child; nn; nn = nn->next) {
		t = print_otag(h, TAG_SPAN, 1, &tag);
		print_text(h, nn->string);
		print_tagq(h, t);

		h->flags |= HTML_NOSPACE;

		if (nn->next && NULL == nn->next->next)
			print_text(h, ", and");
		else if (nn->next)
			print_text(h, ",");
		else
			h->flags &= ~HTML_NOSPACE;
	}

	if (n->child->next)
		print_text(h, "utilities exit");
	else
		print_text(h, "utility exits");

       	print_text(h, "0 on success, and >0 if an error occurs.");
	return(0);
}


/* ARGSUSED */
static int
mdoc_dq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\(lq");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_dq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(rq");
}


/* ARGSUSED */
static int
mdoc_pq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\&(");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_pq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	print_text(h, ")");
}


/* ARGSUSED */
static int
mdoc_sq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\(oq");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_sq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(aq");
}


/* ARGSUSED */
static int
mdoc_em_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "emph";

	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_d1_pre(MDOC_ARGS)
{
	struct htmlpair	tag;
	char		buf[BUFSIZ];

	if (MDOC_BLOCK != n->type)
		return(1);

	(void)snprintf(buf, BUFSIZ - 1, "margin-left: %dpx", 
			INDENT * PX_MULT);

	tag.key = ATTR_STYLE;
	tag.val = buf;

	print_otag(h, TAG_DIV, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_sx_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_HREF;
	tag.val = "#";

	print_otag(h, TAG_A, 1, &tag);
	return(1);
}
