/* $Id: main.c,v 1.8 2009/03/22 19:01:11 kristaps Exp $ */
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
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mdoc.h"

#ifdef __linux__
extern	int		  getsubopt(char **, char * const *, char **);
# ifndef __dead
#  define __dead __attribute__((__noreturn__))
# endif
#endif

struct	buf {
	char	 	 *buf;
	size_t		  sz;
};

struct	curparse {
	const char	 *file;
	int		  wflags;
#define	WARN_WALL	  0x03		/* All-warnings mask. */
#define	WARN_WCOMPAT	 (1 << 0)	/* Compatibility warnings. */
#define	WARN_WSYNTAX	 (1 << 1)	/* Syntax warnings. */
#define	WARN_WERR	 (1 << 2)	/* Warnings->errors. */
};

enum outt {
	OUTT_ASCII,
	OUTT_LATIN1,
	OUTT_UTF8,
	OUTT_TREE,
	OUTT_LINT
};

typedef	int		(*out_run)(void *, const struct mdoc *);
typedef	void		(*out_free)(void *);

extern	char		 *__progname;

extern	void		 *ascii_alloc(void);
extern	void		 *latin1_alloc(void);
extern	void		 *utf8_alloc(void);
extern	int		  terminal_run(void *, const struct mdoc *);
extern	int		  tree_run(void *, const struct mdoc *);
extern	void		  terminal_free(void *);

__dead	static void	  version(void);
__dead	static void	  usage(void);
static	int		  foptions(int *, char *);
static	int		  toptions(enum outt *, char *);
static	int		  woptions(int *, char *);
static	int		  merr(void *, int, int, const char *);
static	int		  mwarn(void *, int, int, 
				enum mdoc_warn, const char *);
static	int		  file(struct buf *, struct buf *,
				const char *, struct mdoc *);
static	int		  fdesc(struct buf *, struct buf *,
				const char *, int, struct mdoc *);


int
main(int argc, char *argv[])
{
	int		 c, rc, fflags;
	struct mdoc_cb	 cb;
	struct mdoc	*mdoc;
	void		*outdata;
	enum outt	 outtype;
	struct buf	 ln, blk;
	out_run		 outrun;
	out_free	 outfree;
	struct curparse	 curp;

	fflags = 0;
	outtype = OUTT_ASCII;

	bzero(&curp, sizeof(struct curparse));

	/* LINTED */
	while (-1 != (c = getopt(argc, argv, "f:VW:T:")))
		switch (c) {
		case ('f'):
			if ( ! foptions(&fflags, optarg))
				return(0);
			break;
		case ('T'):
			if ( ! toptions(&outtype, optarg))
				return(0);
			break;
		case ('W'):
			if ( ! woptions(&curp.wflags, optarg))
				return(0);
			break;
		case ('V'):
			version();
			/* NOTREACHED */
		default:
			usage();
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	/*
	 * Allocate the appropriate front-end.  Note that utf8, ascii
	 * and latin1 all resolve to the terminal front-end with
	 * different encodings (see terminal.c).  Not all frontends have
	 * cleanup or alloc routines.
	 */

	switch (outtype) {
	case (OUTT_LATIN1):
		outdata = latin1_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	case (OUTT_UTF8):
		outdata = utf8_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	case (OUTT_TREE):
		outdata = NULL;
		outrun = tree_run;
		outfree = NULL;
		break;
	case (OUTT_LINT):
		outdata = NULL;
		outrun = NULL;
		outfree = NULL;
		break;
	default:
		outdata = ascii_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	}

	/*
	 * All callbacks route into here, where we print them onto the
	 * screen.  XXX - for now, no path for debugging messages.
	 */

	cb.mdoc_msg = NULL;
	cb.mdoc_err = merr;
	cb.mdoc_warn = mwarn;

	bzero(&ln, sizeof(struct buf));
	bzero(&blk, sizeof(struct buf));

	mdoc = mdoc_alloc(&curp, fflags, &cb);

	/*
	 * Loop around available files.
	 */

	if (NULL == *argv) {
		curp.file = "<stdin>";
		c = fdesc(&blk, &ln, "stdin", STDIN_FILENO, mdoc);
		rc = 0;
		if (c && NULL == outrun)
			rc = 1;
		else if (c && outrun && (*outrun)(outdata, mdoc))
			rc = 1;
	} else {
		while (*argv) {
			curp.file = *argv;
			c = file(&blk, &ln, *argv, mdoc);
			if ( ! c)
				break;
			if (outrun && ! (*outrun)(outdata, mdoc))
				break;
			/* Reset the parser for another file. */
			mdoc_reset(mdoc);
			argv++;
		}
		rc = NULL == *argv;
	}

	if (blk.buf)
		free(blk.buf);
	if (ln.buf)
		free(ln.buf);
	if (outfree)
		(*outfree)(outdata);

	mdoc_free(mdoc);

	return(rc ? EXIT_SUCCESS : EXIT_FAILURE);
}


__dead static void
version(void)
{

	(void)printf("%s %s\n", __progname, VERSION);
	exit(0);
	/* NOTREACHED */
}


__dead static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s\n", __progname);
	exit(1);
	/* NOTREACHED */
}


static int
file(struct buf *blk, struct buf *ln,
		const char *file, struct mdoc *mdoc)
{
	int		 fd, c;

	if (-1 == (fd = open(file, O_RDONLY, 0))) {
		warn("%s", file);
		return(0);
	}

	c = fdesc(blk, ln, file, fd, mdoc);

	if (-1 == close(fd))
		warn("%s", file);

	return(c);
}


static int
fdesc(struct buf *blk, struct buf *ln,
		const char *f, int fd, struct mdoc *mdoc)
{
	size_t		 sz;
	ssize_t		 ssz;
	struct stat	 st;
	int		 j, i, pos, lnn;
#ifdef	STRIP_XO
	int		 macro, xo, xeoln;
#endif

	/*
	 * Two buffers: ln and buf.  buf is the input buffer, optimised
	 * for each file's block size.  ln is a line buffer.  Both
	 * growable, hence passed in by ptr-ptr.
	 */

	sz = BUFSIZ;

	if (-1 == fstat(fd, &st))
		warnx("%s", f);
	else if ((size_t)st.st_blksize > sz)
		sz = st.st_blksize;

	if (sz > blk->sz) {
		blk->buf = realloc(blk->buf, sz);
		if (NULL == blk->buf)
			err(1, "realloc");
		blk->sz = sz;
	}

	/*
	 * Fill buf with file blocksize and parse newlines into ln.
	 */
#ifdef	STRIP_XO
	macro = xo = xeoln = 0;
#endif

	for (lnn = 1, pos = 0; ; ) {
		if (-1 == (ssz = read(fd, blk->buf, sz))) {
			warn("%s", f);
			return(0);
		} else if (0 == ssz) 
			break;

		for (i = 0; i < (int)ssz; i++) {
			if (pos >= (int)ln->sz) {
				ln->sz += 256; /* Step-size. */
				ln->buf = realloc(ln->buf, ln->sz);
				if (NULL == ln->buf)
					err(1, "realloc");
			}

			if ('\n' != blk->buf[i]) {
				/*
				 * Ugly of uglies.  Here we handle the
				 * dreaded `Xo/Xc' scoping.  Cover the
				 * eyes of any nearby children.  This
				 * makes `Xo/Xc' enclosures look like
				 * one huge line.
				 */
#ifdef	STRIP_XO
				/*
				 * First, note whether we're in a macro
				 * line.
				 */
				if (0 == pos && '.' == blk->buf[i])
					macro = 1;

				/*
				 * If we're in an `Xo' context and just
				 * nixed a newline, remove the control
				 * character for new macro lines:
				 * they're going to show up as all part
				 * of the same line.
				 */
				if (xo && xeoln && '.' == blk->buf[i]) {
					xeoln = 0;
					continue;
				}
				xeoln = 0;

				/*
				 * If we've parsed `Xo', enter an xo
				 * context.  `Xo' must be in a parsable
				 * state.  This is the ugly part.  IT IS
				 * NOT SMART ENOUGH TO HANDLE ESCAPED
				 * WHITESPACE.
				 */
				if (macro && pos && 'o' == blk->buf[i]) {
					if (xo && 'X' == ln->buf[pos - 1])  {
						if (' ' == ln->buf[pos - 2])
							xo++;
					} else if ('X' == ln->buf[pos - 1]) {
						if (2 == pos && '.' == ln->buf[pos - 2])
							xo++;
						else if (' ' == ln->buf[pos - 2])
							xo++;
					}
				}

				/*
				 * If we're parsed `Xc', leave an xo
				 * context if one's already pending.
				 * `Xc' must be in a parsable state.
				 * THIS IS NOT SMART ENOUGH TO HANDLE
				 * ESCAPED WHITESPACE.
				 */
				if (macro && pos && 'c' == blk->buf[i])
					if (xo && 'X' == ln->buf[pos - 1])
						if (' ' == ln->buf[pos - 2])
							xo--;
#endif	/* STRIP_XO */

				ln->buf[pos++] = blk->buf[i];
				continue;
			}

			/* Check for CPP-escaped newline.  */

			if (pos > 0 && '\\' == ln->buf[pos - 1]) {
				for (j = pos - 1; j >= 0; j--)
					if ('\\' != ln->buf[j])
						break;

				if ( ! ((pos - j) % 2)) {
					pos--;
					lnn++;
					continue;
				}
			}

#ifdef	STRIP_XO
			/*
			 * If we're in an xo context, put a space in
			 * place of the newline and continue parsing.
			 * Mark that we just did a newline.
			 */
			if (xo) {
				xeoln = 1;
				ln->buf[pos++] = ' ';
				lnn++;
				continue;
			}
			macro = 0;
#endif	/* STRIP_XO */

			ln->buf[pos] = 0;
			if ( ! mdoc_parseln(mdoc, lnn, ln->buf))
				return(0);
			lnn++;
			pos = 0;
		}
	}

	return(mdoc_endparse(mdoc));
}


static int
toptions(enum outt *tflags, char *arg)
{

	if (0 == strcmp(arg, "ascii"))
		*tflags = OUTT_ASCII;
	else if (0 == strcmp(arg, "latin1"))
		*tflags = OUTT_LATIN1;
	else if (0 == strcmp(arg, "utf8"))
		*tflags = OUTT_UTF8;
	else if (0 == strcmp(arg, "lint"))
		*tflags = OUTT_LINT;
	else if (0 == strcmp(arg, "tree"))
		*tflags = OUTT_TREE;
	else {
		warnx("bad argument: -T%s", arg);
		return(0);
	}

	return(1);
}


/*
 * Parse out the options for [-fopt...] setting compiler options.  These
 * can be comma-delimited or called again.
 */
static int
foptions(int *fflags, char *arg)
{
	char		*v;
	char		*toks[4];

	toks[0] = "ign-scope";
	toks[1] = "ign-escape";
	toks[2] = "ign-macro";
	toks[3] = NULL;

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			*fflags |= MDOC_IGN_SCOPE;
			break;
		case (1):
			*fflags |= MDOC_IGN_ESCAPE;
			break;
		case (2):
			*fflags |= MDOC_IGN_MACRO;
			break;
		default:
			warnx("bad argument: -f%s", arg);
			return(0);
		}

	return(1);
}


/* 
 * Parse out the options for [-Werr...], which sets warning modes.
 * These can be comma-delimited or called again.  
 */
static int
woptions(int *wflags, char *arg)
{
	char		*v;
	char		*toks[5]; 

	toks[0] = "all";
	toks[1] = "compat";
	toks[2] = "syntax";
	toks[3] = "error";
	toks[4] = NULL;

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			*wflags |= WARN_WALL;
			break;
		case (1):
			*wflags |= WARN_WCOMPAT;
			break;
		case (2):
			*wflags |= WARN_WSYNTAX;
			break;
		case (3):
			*wflags |= WARN_WERR;
			break;
		default:
			warnx("bad argument: -W%s", arg);
			return(0);
		}

	return(1);
}


/* ARGSUSED */
static int
merr(void *arg, int line, int col, const char *msg)
{
	struct curparse *curp;

	curp = (struct curparse *)arg;

	warnx("%s:%d: error: %s (column %d)", 
			curp->file, line, msg, col);
	return(0);
}


static int
mwarn(void *arg, int line, int col, 
		enum mdoc_warn type, const char *msg)
{
	struct curparse *curp;
	char		*wtype;

	curp = (struct curparse *)arg;
	wtype = NULL;

	switch (type) {
	case (WARN_COMPAT):
		wtype = "compat";
		if (curp->wflags & WARN_WCOMPAT)
			break;
		return(1);
	case (WARN_SYNTAX):
		wtype = "syntax";
		if (curp->wflags & WARN_WSYNTAX)
			break;
		return(1);
	}

	assert(wtype);
	warnx("%s:%d: %s warning: %s (column %d)", 
			curp->file, line, wtype, msg, col);

	if ( ! (curp->wflags & WARN_WERR))
		return(1);

	warnx("%s: considering warnings as errors", 
			__progname);
	return(0);
}


