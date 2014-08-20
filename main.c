/*	$Id: main.c,v 1.180 2014/08/17 03:24:47 schwarze Exp $ */
/*
 * Copyright (c) 2008, 2009, 2010, 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2010, 2011, 2012, 2014 Ingo Schwarze <schwarze@openbsd.org>
 * Copyright (c) 2010 Joerg Sonnenberger <joerg@netbsd.org>
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
#include "config.h"

#include <sys/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mandoc.h"
#include "mandoc_aux.h"
#include "main.h"
#include "mdoc.h"
#include "man.h"
#include "manpath.h"
#include "mansearch.h"

#if !defined(__GNUC__) || (__GNUC__ < 2)
# if !defined(lint)
#  define __attribute__(x)
# endif
#endif /* !defined(__GNUC__) || (__GNUC__ < 2) */

enum	outmode {
	OUTMODE_DEF = 0,
	OUTMODE_FLN,
	OUTMODE_LST,
	OUTMODE_ALL,
	OUTMODE_INT,
	OUTMODE_ONE
};

typedef	void		(*out_mdoc)(void *, const struct mdoc *);
typedef	void		(*out_man)(void *, const struct man *);
typedef	void		(*out_free)(void *);

enum	outt {
	OUTT_ASCII = 0,	/* -Tascii */
	OUTT_LOCALE,	/* -Tlocale */
	OUTT_UTF8,	/* -Tutf8 */
	OUTT_TREE,	/* -Ttree */
	OUTT_MAN,	/* -Tman */
	OUTT_HTML,	/* -Thtml */
	OUTT_XHTML,	/* -Txhtml */
	OUTT_LINT,	/* -Tlint */
	OUTT_PS,	/* -Tps */
	OUTT_PDF	/* -Tpdf */
};

struct	curparse {
	struct mparse	 *mp;
	enum mandoclevel  wlevel;	/* ignore messages below this */
	int		  wstop;	/* stop after a file with a warning */
	enum outt	  outtype;	/* which output to use */
	out_mdoc	  outmdoc;	/* mdoc output ptr */
	out_man		  outman;	/* man output ptr */
	out_free	  outfree;	/* free output ptr */
	void		 *outdata;	/* data for output */
	char		  outopts[BUFSIZ]; /* buf of output opts */
};

static	int		  moptions(int *, char *);
static	void		  mmsg(enum mandocerr, enum mandoclevel,
				const char *, int, int, const char *);
static	void		  parse(struct curparse *, int,
				const char *, enum mandoclevel *);
static	int		  toptions(struct curparse *, char *);
static	void		  usage(enum argmode) __attribute__((noreturn));
static	void		  version(void) __attribute__((noreturn));
static	int		  woptions(struct curparse *, char *);

static	const char	 *progname;


int
main(int argc, char *argv[])
{
	struct curparse	 curp;
	struct mansearch search;
	struct manpaths	 paths;
	char		*conf_file, *defpaths, *auxpaths;
	char		*defos;
#if HAVE_SQLITE3
	struct manpage	*res;
	size_t		 i, sz;
#endif
	enum mandoclevel rc;
	enum outmode	 outmode;
	int		 show_usage;
	int		 options;
	int		 c;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	/* Search options. */

	memset(&paths, 0, sizeof(struct manpaths));
	conf_file = defpaths = auxpaths = NULL;

	memset(&search, 0, sizeof(struct mansearch));
	search.outkey = "Nd";

	if (strcmp(progname, "man") == 0)
		search.argmode = ARG_NAME;
	else if (strncmp(progname, "apropos", 7) == 0)
		search.argmode = ARG_EXPR;
	else if (strncmp(progname, "whatis", 6) == 0)
		search.argmode = ARG_WORD;
	else
		search.argmode = ARG_FILE;

	/* Parser and formatter options. */

	memset(&curp, 0, sizeof(struct curparse));
	curp.outtype = OUTT_ASCII;
	curp.wlevel  = MANDOCLEVEL_FATAL;
	options = MPARSE_SO;
	defos = NULL;

	show_usage = 0;
	outmode = OUTMODE_DEF;
	while (-1 != (c = getopt(argc, argv, "aC:fI:ikM:m:O:S:s:T:VW:w"))) {
		switch (c) {
		case 'a':
			outmode = OUTMODE_ALL;
			break;
		case 'C':
			conf_file = optarg;
			break;
		case 'f':
			search.argmode = ARG_WORD;
			break;
		case 'I':
			if (strncmp(optarg, "os=", 3)) {
				fprintf(stderr,
				    "%s: -I%s: Bad argument\n",
				    progname, optarg);
				return((int)MANDOCLEVEL_BADARG);
			}
			if (defos) {
				fprintf(stderr,
				    "%s: -I%s: Duplicate argument\n",
				    progname, optarg);
				return((int)MANDOCLEVEL_BADARG);
			}
			defos = mandoc_strdup(optarg + 3);
			break;
		case 'i':
			outmode = OUTMODE_INT;
			break;
		case 'k':
			search.argmode = ARG_EXPR;
			break;
		case 'M':
			defpaths = optarg;
			break;
		case 'm':
			auxpaths = optarg;
			break;
		case 'O':
			search.outkey = optarg;
			(void)strlcat(curp.outopts, optarg, BUFSIZ);
			(void)strlcat(curp.outopts, ",", BUFSIZ);
			break;
		case 'S':
			search.arch = optarg;
			break;
		case 's':
			search.sec = optarg;
			break;
		case 'T':
			if ( ! toptions(&curp, optarg))
				return((int)MANDOCLEVEL_BADARG);
			break;
		case 'W':
			if ( ! woptions(&curp, optarg))
				return((int)MANDOCLEVEL_BADARG);
			break;
		case 'w':
			outmode = OUTMODE_FLN;
			break;
		case 'V':
			version();
			/* NOTREACHED */
		default:
			show_usage = 1;
			break;
		}
	}

	if (show_usage)
		usage(search.argmode);

	if (outmode == OUTMODE_DEF) {
		switch (search.argmode) {
		case ARG_FILE:
			outmode = OUTMODE_ALL;
			break;
		case ARG_NAME:
			outmode = OUTMODE_ONE;
			break;
		default:
			outmode = OUTMODE_LST;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	/* man(1), whatis(1), apropos(1) */

	if (search.argmode != ARG_FILE) {
#if HAVE_SQLITE3
		if (argc == 0)
			usage(search.argmode);
		manpath_parse(&paths, conf_file, defpaths, auxpaths);
		mansearch_setup(1);
		if( ! mansearch(&search, &paths, argc, argv, &res, &sz))
			usage(search.argmode);
		manpath_free(&paths);
		for (i = 0; i < sz; i++) {
			if (outmode == OUTMODE_FLN)
				puts(res[i].file);
			else
				printf("%s - %s\n", res[i].names,
				    res[i].output == NULL ? "" :
				    res[i].output);
		}
		mansearch_free(res, sz);
		mansearch_setup(0);
		return((int)MANDOCLEVEL_OK);
#else
		fputs("mandoc: database support not compiled in\n",
		    stderr);
		return((int)MANDOCLEVEL_BADARG);
#endif
	}

	/* mandoc(1) */

	if ( ! moptions(&options, auxpaths))
		return((int)MANDOCLEVEL_BADARG);

	curp.mp = mparse_alloc(options, curp.wlevel, mmsg, defos);

	/*
	 * Conditionally start up the lookaside buffer before parsing.
	 */
	if (OUTT_MAN == curp.outtype)
		mparse_keep(curp.mp);

	rc = MANDOCLEVEL_OK;

	if (NULL == *argv)
		parse(&curp, STDIN_FILENO, "<stdin>", &rc);

	while (*argv) {
		parse(&curp, -1, *argv, &rc);
		if (MANDOCLEVEL_OK != rc && curp.wstop)
			break;
		++argv;
	}

	if (curp.outfree)
		(*curp.outfree)(curp.outdata);
	if (curp.mp)
		mparse_free(curp.mp);
	free(defos);

	return((int)rc);
}

static void
version(void)
{

	printf("mandoc %s\n", VERSION);
	exit((int)MANDOCLEVEL_OK);
}

static void
usage(enum argmode argmode)
{

	switch (argmode) {
	case ARG_FILE:
		fputs("usage: mandoc [-V] [-Ios=name] [-mformat]"
		    " [-Ooption] [-Toutput] [-Wlevel]\n"
		    "\t      [file ...]\n", stderr);
		break;
	case ARG_NAME:
		fputs("usage: man [-acfhkVw] [-C file] "
		    "[-M path] [-m path] [-S arch] [-s section]\n"
		    "\t   [section] name ...\n", stderr);
		break;
	case ARG_WORD:
		fputs("usage: whatis [-V] [-C file] [-M path] [-m path] "
		    "[-S arch] [-s section] name ...\n", stderr);
		break;
	case ARG_EXPR:
		fputs("usage: apropos [-V] [-C file] [-M path] [-m path] "
		    "[-O outkey] [-S arch]\n"
		    "\t       [-s section] expression ...\n", stderr);
		break;
	}
	exit((int)MANDOCLEVEL_BADARG);
}

static void
parse(struct curparse *curp, int fd, const char *file,
	enum mandoclevel *level)
{
	enum mandoclevel  rc;
	struct mdoc	 *mdoc;
	struct man	 *man;

	/* Begin by parsing the file itself. */

	assert(file);
	assert(fd >= -1);

	rc = mparse_readfd(curp->mp, fd, file);

	/* Stop immediately if the parse has failed. */

	if (MANDOCLEVEL_FATAL <= rc)
		goto cleanup;

	/*
	 * With -Wstop and warnings or errors of at least the requested
	 * level, do not produce output.
	 */

	if (MANDOCLEVEL_OK != rc && curp->wstop)
		goto cleanup;

	/* If unset, allocate output dev now (if applicable). */

	if ( ! (curp->outman && curp->outmdoc)) {
		switch (curp->outtype) {
		case OUTT_XHTML:
			curp->outdata = xhtml_alloc(curp->outopts);
			curp->outfree = html_free;
			break;
		case OUTT_HTML:
			curp->outdata = html_alloc(curp->outopts);
			curp->outfree = html_free;
			break;
		case OUTT_UTF8:
			curp->outdata = utf8_alloc(curp->outopts);
			curp->outfree = ascii_free;
			break;
		case OUTT_LOCALE:
			curp->outdata = locale_alloc(curp->outopts);
			curp->outfree = ascii_free;
			break;
		case OUTT_ASCII:
			curp->outdata = ascii_alloc(curp->outopts);
			curp->outfree = ascii_free;
			break;
		case OUTT_PDF:
			curp->outdata = pdf_alloc(curp->outopts);
			curp->outfree = pspdf_free;
			break;
		case OUTT_PS:
			curp->outdata = ps_alloc(curp->outopts);
			curp->outfree = pspdf_free;
			break;
		default:
			break;
		}

		switch (curp->outtype) {
		case OUTT_HTML:
			/* FALLTHROUGH */
		case OUTT_XHTML:
			curp->outman = html_man;
			curp->outmdoc = html_mdoc;
			break;
		case OUTT_TREE:
			curp->outman = tree_man;
			curp->outmdoc = tree_mdoc;
			break;
		case OUTT_MAN:
			curp->outmdoc = man_mdoc;
			curp->outman = man_man;
			break;
		case OUTT_PDF:
			/* FALLTHROUGH */
		case OUTT_ASCII:
			/* FALLTHROUGH */
		case OUTT_UTF8:
			/* FALLTHROUGH */
		case OUTT_LOCALE:
			/* FALLTHROUGH */
		case OUTT_PS:
			curp->outman = terminal_man;
			curp->outmdoc = terminal_mdoc;
			break;
		default:
			break;
		}
	}

	mparse_result(curp->mp, &mdoc, &man, NULL);

	/* Execute the out device, if it exists. */

	if (man && curp->outman)
		(*curp->outman)(curp->outdata, man);
	if (mdoc && curp->outmdoc)
		(*curp->outmdoc)(curp->outdata, mdoc);

 cleanup:

	mparse_reset(curp->mp);

	if (*level < rc)
		*level = rc;
}

static int
moptions(int *options, char *arg)
{

	if (arg == NULL)
		/* nothing to do */;
	else if (0 == strcmp(arg, "doc"))
		*options |= MPARSE_MDOC;
	else if (0 == strcmp(arg, "andoc"))
		/* nothing to do */;
	else if (0 == strcmp(arg, "an"))
		*options |= MPARSE_MAN;
	else {
		fprintf(stderr, "%s: -m%s: Bad argument\n",
		    progname, arg);
		return(0);
	}

	return(1);
}

static int
toptions(struct curparse *curp, char *arg)
{

	if (0 == strcmp(arg, "ascii"))
		curp->outtype = OUTT_ASCII;
	else if (0 == strcmp(arg, "lint")) {
		curp->outtype = OUTT_LINT;
		curp->wlevel  = MANDOCLEVEL_WARNING;
	} else if (0 == strcmp(arg, "tree"))
		curp->outtype = OUTT_TREE;
	else if (0 == strcmp(arg, "man"))
		curp->outtype = OUTT_MAN;
	else if (0 == strcmp(arg, "html"))
		curp->outtype = OUTT_HTML;
	else if (0 == strcmp(arg, "utf8"))
		curp->outtype = OUTT_UTF8;
	else if (0 == strcmp(arg, "locale"))
		curp->outtype = OUTT_LOCALE;
	else if (0 == strcmp(arg, "xhtml"))
		curp->outtype = OUTT_XHTML;
	else if (0 == strcmp(arg, "ps"))
		curp->outtype = OUTT_PS;
	else if (0 == strcmp(arg, "pdf"))
		curp->outtype = OUTT_PDF;
	else {
		fprintf(stderr, "%s: -T%s: Bad argument\n",
		    progname, arg);
		return(0);
	}

	return(1);
}

static int
woptions(struct curparse *curp, char *arg)
{
	char		*v, *o;
	const char	*toks[6];

	toks[0] = "stop";
	toks[1] = "all";
	toks[2] = "warning";
	toks[3] = "error";
	toks[4] = "fatal";
	toks[5] = NULL;

	while (*arg) {
		o = arg;
		switch (getsubopt(&arg, UNCONST(toks), &v)) {
		case 0:
			curp->wstop = 1;
			break;
		case 1:
			/* FALLTHROUGH */
		case 2:
			curp->wlevel = MANDOCLEVEL_WARNING;
			break;
		case 3:
			curp->wlevel = MANDOCLEVEL_ERROR;
			break;
		case 4:
			curp->wlevel = MANDOCLEVEL_FATAL;
			break;
		default:
			fprintf(stderr, "%s: -W%s: Bad argument\n",
			    progname, o);
			return(0);
		}
	}

	return(1);
}

static void
mmsg(enum mandocerr t, enum mandoclevel lvl,
		const char *file, int line, int col, const char *msg)
{
	const char	*mparse_msg;

	fprintf(stderr, "%s: %s:", progname, file);

	if (line)
		fprintf(stderr, "%d:%d:", line, col + 1);

	fprintf(stderr, " %s", mparse_strlevel(lvl));

	if (NULL != (mparse_msg = mparse_strerror(t)))
		fprintf(stderr, ": %s", mparse_msg);

	if (msg)
		fprintf(stderr, ": %s", msg);

	fputc('\n', stderr);
}
