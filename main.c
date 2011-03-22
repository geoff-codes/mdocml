/*	$Id: main.c,v 1.157 2011/03/21 12:04:26 kristaps Exp $ */
/*
 * Copyright (c) 2008, 2009, 2010, 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2010, 2011 Ingo Schwarze <schwarze@openbsd.org>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mandoc.h"
#include "main.h"
#include "mdoc.h"
#include "man.h"

#if !defined(__GNUC__) || (__GNUC__ < 2)
# if !defined(lint)
#  define __attribute__(x)
# endif
#endif /* !defined(__GNUC__) || (__GNUC__ < 2) */

typedef	void		(*out_mdoc)(void *, const struct mdoc *);
typedef	void		(*out_man)(void *, const struct man *);
typedef	void		(*out_free)(void *);

enum	outt {
	OUTT_ASCII = 0,	/* -Tascii */
	OUTT_TREE,	/* -Ttree */
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
	enum outt	  outtype; 	/* which output to use */
	out_mdoc	  outmdoc;	/* mdoc output ptr */
	out_man	  	  outman;	/* man output ptr */
	out_free	  outfree;	/* free output ptr */
	void		 *outdata;	/* data for output */
	char		  outopts[BUFSIZ]; /* buf of output opts */
};

static	const char * const	mandoclevels[MANDOCLEVEL_MAX] = {
	"SUCCESS",
	"RESERVED",
	"WARNING",
	"ERROR",
	"FATAL",
	"BADARG",
	"SYSERR"
};

static	const char * const	mandocerrs[MANDOCERR_MAX] = {
	"ok",

	"generic warning",

	/* related to the prologue */
	"no title in document",
	"document title should be all caps",
	"unknown manual section",
	"date missing, using today's date",
	"cannot parse date, using it verbatim",
	"prologue macros out of order",
	"duplicate prologue macro",
	"macro not allowed in prologue",
	"macro not allowed in body",

	/* related to document structure */
	".so is fragile, better use ln(1)",
	"NAME section must come first",
	"bad NAME section contents",
	"manual name not yet set",
	"sections out of conventional order",
	"duplicate section name",
	"section not in conventional manual section",

	/* related to macros and nesting */
	"skipping obsolete macro",
	"skipping paragraph macro",
	"skipping no-space macro",
	"blocks badly nested",
	"child violates parent syntax",
	"nested displays are not portable",
	"already in literal mode",

	/* related to missing macro arguments */
	"skipping empty macro",
	"argument count wrong",
	"missing display type",
	"list type must come first",
	"tag lists require a width argument",
	"missing font type",
	"skipping end of block that is not open",

	/* related to bad macro arguments */
	"skipping argument",
	"duplicate argument",
	"duplicate display type",
	"duplicate list type",
	"unknown AT&T UNIX version",
	"bad Boolean value",
	"unknown font",
	"unknown standard specifier",
	"bad width argument",

	/* related to plain text */
	"blank line in non-literal context",
	"tab in non-literal context",
	"end of line whitespace",
	"bad comment style",
	"unknown escape sequence",
	"unterminated quoted string",
	
	"generic error",

	/* related to tables */
	"bad table syntax",
	"bad table option",
	"bad table layout",
	"no table layout cells specified",
	"no table data cells specified",
	"ignore data in cell",
	"data block still open",
	"ignoring extra data cells",

	"input stack limit exceeded, infinite loop?",
	"skipping bad character",
	"escaped character not allowed in a name",
	"skipping text before the first section header",
	"skipping unknown macro",
	"NOT IMPLEMENTED, please use groff: skipping request",
	"line scope broken",
	"argument count wrong",
	"skipping end of block that is not open",
	"missing end of block",
	"scope open on exit",
	"uname(3) system call failed",
	"macro requires line argument(s)",
	"macro requires body argument(s)",
	"macro requires argument(s)",
	"missing list type",
	"line argument(s) will be lost",
	"body argument(s) will be lost",

	"generic fatal error",

	"not a manual",
	"column syntax is inconsistent",
	"NOT IMPLEMENTED: .Bd -file",
	"line scope broken, syntax violated",
	"argument count wrong, violates syntax",
	"child violates parent syntax",
	"argument count wrong, violates syntax",
	"NOT IMPLEMENTED: .so with absolute path or \"..\"",
	"no document body",
	"no document prologue",
	"static buffer exhausted",
};

static	int		  moptions(enum mparset *, char *);
static	void		  mmsg(enum mandocerr, enum mandoclevel,
				const char *, int, int, const char *);
static	void		  parse(struct curparse *, int, 
				const char *, enum mandoclevel *);
static	int		  toptions(struct curparse *, char *);
static	void		  usage(void) __attribute__((noreturn));
static	void		  version(void) __attribute__((noreturn));
static	int		  woptions(struct curparse *, char *);

static	const char	 *progname;

int
main(int argc, char *argv[])
{
	int		 c;
	struct curparse	 curp;
	enum mparset	 type;
	enum mandoclevel rc;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	memset(&curp, 0, sizeof(struct curparse));

	type = MPARSE_AUTO;
	curp.outtype = OUTT_ASCII;
	curp.wlevel  = MANDOCLEVEL_FATAL;

	/* LINTED */
	while (-1 != (c = getopt(argc, argv, "m:O:T:VW:")))
		switch (c) {
		case ('m'):
			if ( ! moptions(&type, optarg))
				return((int)MANDOCLEVEL_BADARG);
			break;
		case ('O'):
			(void)strlcat(curp.outopts, optarg, BUFSIZ);
			(void)strlcat(curp.outopts, ",", BUFSIZ);
			break;
		case ('T'):
			if ( ! toptions(&curp, optarg))
				return((int)MANDOCLEVEL_BADARG);
			break;
		case ('W'):
			if ( ! woptions(&curp, optarg))
				return((int)MANDOCLEVEL_BADARG);
			break;
		case ('V'):
			version();
			/* NOTREACHED */
		default:
			usage();
			/* NOTREACHED */
		}

	curp.mp = mparse_alloc(type, curp.wlevel, mmsg, &curp);

	argc -= optind;
	argv += optind;

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

	return((int)rc);
}

static void
version(void)
{

	printf("%s %s\n", progname, VERSION);
	exit((int)MANDOCLEVEL_OK);
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s "
			"[-V] "
			"[-foption] "
			"[-mformat] "
			"[-Ooption] "
			"[-Toutput] "
			"[-Werr] "
			"[file...]\n", 
			progname);

	exit((int)MANDOCLEVEL_BADARG);
}

static void
parse(struct curparse *curp, int fd, 
		const char *file, enum mandoclevel *level)
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
		case (OUTT_XHTML):
			curp->outdata = xhtml_alloc(curp->outopts);
			break;
		case (OUTT_HTML):
			curp->outdata = html_alloc(curp->outopts);
			break;
		case (OUTT_ASCII):
			curp->outdata = ascii_alloc(curp->outopts);
			curp->outfree = ascii_free;
			break;
		case (OUTT_PDF):
			curp->outdata = pdf_alloc(curp->outopts);
			curp->outfree = pspdf_free;
			break;
		case (OUTT_PS):
			curp->outdata = ps_alloc(curp->outopts);
			curp->outfree = pspdf_free;
			break;
		default:
			break;
		}

		switch (curp->outtype) {
		case (OUTT_HTML):
			/* FALLTHROUGH */
		case (OUTT_XHTML):
			curp->outman = html_man;
			curp->outmdoc = html_mdoc;
			curp->outfree = html_free;
			break;
		case (OUTT_TREE):
			curp->outman = tree_man;
			curp->outmdoc = tree_mdoc;
			break;
		case (OUTT_PDF):
			/* FALLTHROUGH */
		case (OUTT_ASCII):
			/* FALLTHROUGH */
		case (OUTT_PS):
			curp->outman = terminal_man;
			curp->outmdoc = terminal_mdoc;
			break;
		default:
			break;
		}
	}

	mparse_result(curp->mp, &mdoc, &man);

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
moptions(enum mparset *tflags, char *arg)
{

	if (0 == strcmp(arg, "doc"))
		*tflags = MPARSE_MDOC;
	else if (0 == strcmp(arg, "andoc"))
		*tflags = MPARSE_AUTO;
	else if (0 == strcmp(arg, "an"))
		*tflags = MPARSE_MAN;
	else {
		fprintf(stderr, "%s: Bad argument\n", arg);
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
	else if (0 == strcmp(arg, "html"))
		curp->outtype = OUTT_HTML;
	else if (0 == strcmp(arg, "xhtml"))
		curp->outtype = OUTT_XHTML;
	else if (0 == strcmp(arg, "ps"))
		curp->outtype = OUTT_PS;
	else if (0 == strcmp(arg, "pdf"))
		curp->outtype = OUTT_PDF;
	else {
		fprintf(stderr, "%s: Bad argument\n", arg);
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
		case (0):
			curp->wstop = 1;
			break;
		case (1):
			/* FALLTHROUGH */
		case (2):
			curp->wlevel = MANDOCLEVEL_WARNING;
			break;
		case (3):
			curp->wlevel = MANDOCLEVEL_ERROR;
			break;
		case (4):
			curp->wlevel = MANDOCLEVEL_FATAL;
			break;
		default:
			fprintf(stderr, "-W%s: Bad argument\n", o);
			return(0);
		}
	}

	return(1);
}

static void
mmsg(enum mandocerr t, enum mandoclevel lvl, 
		const char *file, int line, int col, const char *msg)
{

	fprintf(stderr, "%s:%d:%d: %s: %s", 
			file, line, col + 1, 
			mandoclevels[lvl], mandocerrs[t]);

	if (msg)
		fprintf(stderr, ": %s", msg);

	fputc('\n', stderr);
}
