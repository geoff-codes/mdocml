/* $Id: main.c,v 1.20 2009/04/02 16:37:40 kristaps Exp $ */
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
#include "man.h"

/* Account for FreeBSD and Linux in our declarations. */

#ifdef __linux__
extern	int		  getsubopt(char **, char * const *, char **);
# ifndef __dead
#  define __dead __attribute__((__noreturn__))
# endif
#elif defined(__dead2)
# ifndef __dead
#  define __dead __dead2
# endif
#endif

struct	buf {
	char	 	 *buf;
	size_t		  sz;
};

enum	intt {
	INTT_AUTO,
	INTT_MDOC,
	INTT_MAN
};

enum	outt {
	OUTT_ASCII = 0,
	OUTT_TREE,
	OUTT_LINT
};

struct	curparse {
	const char	 *file;
	int		  fd;
	int		  wflags;
#define	WARN_WALL	  0x03		/* All-warnings mask. */
#define	WARN_WCOMPAT	 (1 << 0)	/* Compatibility warnings. */
#define	WARN_WSYNTAX	 (1 << 1)	/* Syntax warnings. */
#define	WARN_WERR	 (1 << 2)	/* Warnings->errors. */
	int		  fflags;
	enum intt	  inttype;
	struct man	 *man;
	struct mdoc	 *mdoc;
};

#define	IGN_SCOPE	 (1 << 0) 	/* Ignore scope errors. */
#define	IGN_ESCAPE	 (1 << 1) 	/* Ignore bad escapes. */
#define	IGN_MACRO	 (1 << 2) 	/* Ignore unknown macros. */
#define	NO_IGN_MACRO	 (1 << 3) 

typedef	int		(*out_run)(void *, const struct man *,
				const struct mdoc *);
typedef	void		(*out_free)(void *);

extern	char		 *__progname;

extern	void		 *ascii_alloc(void);
extern	int		  terminal_run(void *, const struct man *, 
				const struct mdoc *);
extern	int		  tree_run(void *, const struct man *,
				const struct mdoc *);
extern	void		  terminal_free(void *);

static	int		  foptions(int *, char *);
static	int		  toptions(enum outt *, char *);
static	int		  moptions(enum intt *, char *);
static	int		  woptions(int *, char *);
static	int		  merr(void *, int, int, const char *);
static	int		  manwarn(void *, int, int, const char *);
static	int		  mdocwarn(void *, int, int, 
				enum mdoc_warn, const char *);
static	int		  fstdin(struct buf *, struct buf *, 
				struct curparse *);
static	int		  ffile(struct buf *, struct buf *, 
				const char *, struct curparse *);
static	int		  fdesc(struct buf *, struct buf *,
				struct curparse *);
static	int		  pset(const char *, size_t, struct curparse *,
				struct man **, struct mdoc **);
static	struct man	 *man_init(struct curparse *);
static	struct mdoc	 *mdoc_init(struct curparse *);
__dead	static void	  version(void);
__dead	static void	  usage(void);


int
main(int argc, char *argv[])
{
	int		 c, rc;
	void		*outdata;
	enum outt	 outtype;
	struct buf	 ln, blk;
	out_run		 outrun;
	out_free	 outfree;
	struct curparse	 curp;

	outtype = OUTT_ASCII;

	bzero(&curp, sizeof(struct curparse));

	curp.inttype = INTT_AUTO;

	/* LINTED */
	while (-1 != (c = getopt(argc, argv, "f:m:VW:T:")))
		switch (c) {
		case ('f'):
			if ( ! foptions(&curp.fflags, optarg))
				return(0);
			break;
		case ('m'):
			if ( ! moptions(&curp.inttype, optarg))
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
	 * Allocate the appropriate front-end.  Note that utf8, latin1
	 * (both not yet implemented) and ascii all resolve to the
	 * terminal front-end with different encodings (see terminal.c).
	 * Not all frontends have cleanup or alloc routines.
	 */

	switch (outtype) {
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

	/* Configure buffers. */

	bzero(&ln, sizeof(struct buf));
	bzero(&blk, sizeof(struct buf));

	/*
	 * Main loop around available files.
	 */

	if (NULL == *argv) {
		rc = 0;
		c = fstdin(&blk, &ln, &curp);

		if (c && NULL == outrun)
			rc = 1;
		/*else if (c && outrun && (*outrun)(outdata, curp.man, curp.mdoc))
			rc = 1;*/
	} else {
		while (*argv) {
			c = ffile(&blk, &ln, *argv, &curp);
			if ( ! c)
				break;
			/*if (outrun && ! (*outrun)(outdata, curp.man, curp.mdoc))
				break;*/
			if (curp.man)
				man_reset(curp.man);
			if (curp.mdoc && ! mdoc_reset(curp.mdoc)) {
				warnx("memory exhausted");
				break;
			}
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
	if (curp.mdoc)
		mdoc_free(curp.mdoc);
	if (curp.man)
		man_free(curp.man);

	return(rc ? EXIT_SUCCESS : EXIT_FAILURE);
}


__dead static void
version(void)
{

	(void)printf("%s %s\n", __progname, VERSION);
	exit(EXIT_SUCCESS);
}


__dead static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-V] [-foption...] "
			"[-mformat] [-Toutput] [-Werr...]\n", 
			__progname);
	exit(EXIT_FAILURE);
}


static struct man *
man_init(struct curparse *curp)
{
	int		 pflags;
	struct man	*man;
	struct man_cb	 mancb;

	mancb.man_err = merr;
	mancb.man_warn = manwarn;

	/* Set command defaults. */
	pflags = MAN_IGN_MACRO;

	if (curp->fflags & NO_IGN_MACRO)
		pflags &= ~MAN_IGN_MACRO;

	if (NULL == (man = man_alloc(curp, pflags, &mancb)))
		warnx("memory exhausted");

	return(man);
}


static struct mdoc *
mdoc_init(struct curparse *curp)
{
	int		 pflags;
	struct mdoc	*mdoc;
	struct mdoc_cb	 mdoccb;

	mdoccb.mdoc_msg = NULL;
	mdoccb.mdoc_err = merr;
	mdoccb.mdoc_warn = mdocwarn;

	pflags = 0;

	if (curp->fflags & IGN_SCOPE)
		pflags |= MDOC_IGN_SCOPE;
	if (curp->fflags & IGN_ESCAPE)
		pflags |= MDOC_IGN_ESCAPE;
	if (curp->fflags & IGN_MACRO)
		pflags |= MDOC_IGN_MACRO;

	if (NULL == (mdoc = mdoc_alloc(curp, pflags, &mdoccb)))
		warnx("memory allocated");

	return(mdoc);
}


static int
fstdin(struct buf *blk, struct buf *ln, struct curparse *curp)
{

	curp->file = "<stdin>";
	curp->fd = STDIN_FILENO;
	return(fdesc(blk, ln, curp));
}


static int
ffile(struct buf *blk, struct buf *ln, 
		const char *file, struct curparse *curp)
{
	int		 c;

	curp->file = file;
	if (-1 == (curp->fd = open(curp->file, O_RDONLY, 0))) {
		warn("%s", curp->file);
		return(0);
	}

	c = fdesc(blk, ln, curp);

	if (-1 == close(curp->fd))
		warn("%s", curp->file);

	return(c);
}


static int
fdesc(struct buf *blk, struct buf *ln, struct curparse *curp)
{
	size_t		 sz;
	ssize_t		 ssz;
	struct stat	 st;
	int		 j, i, pos, lnn;
	struct man	*man;
	struct mdoc	*mdoc;

	sz = BUFSIZ;
	man = NULL;
	mdoc = NULL;

	/*
	 * Two buffers: ln and buf.  buf is the input buffer optimised
	 * here for each file's block size.  ln is a line buffer.  Both
	 * growable, hence passed in by ptr-ptr.
	 */

	if (-1 == fstat(curp->fd, &st))
		warnx("%s", curp->file);
	else if ((size_t)st.st_blksize > sz)
		sz = st.st_blksize;

	if (sz > blk->sz) {
		blk->buf = realloc(blk->buf, sz);
		if (NULL == blk->buf) {
			warn("realloc");
			return(0);
		}
		blk->sz = sz;
	}

	/* Fill buf with file blocksize. */

	for (lnn = 0, pos = 0; ; ) {
		if (-1 == (ssz = read(curp->fd, blk->buf, sz))) {
			warn("%s", curp->file);
			return(0);
		} else if (0 == ssz) 
			break;

		/* Parse the read block into partial or full lines. */

		for (i = 0; i < (int)ssz; i++) {
			if (pos >= (int)ln->sz) {
				ln->sz += 256; /* Step-size. */
				ln->buf = realloc(ln->buf, ln->sz);
				if (NULL == ln->buf) {
					warn("realloc");
					return(0);
				}
			}

			if ('\n' != blk->buf[i]) {
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

			ln->buf[pos] = 0;
			lnn++;
			
			/*
			 * If no manual parser has been assigned, then
			 * try to assign one in pset(), which may do
			 * nothing at all.  After this, parse the manual
			 * line accordingly.
			 */

			if ( ! (man || mdoc) && ! pset(ln->buf, 
						pos, curp, &man, &mdoc))
				return(0);

			pos = 0;

			if (man && ! man_parseln(man, lnn, ln->buf))
				return(0);
			if (mdoc && ! mdoc_parseln(mdoc, lnn, ln->buf))
				return(0);
		}
	}

	/* Note that a parser may not have been assigned, yet. */

	if (mdoc)
	       return(mdoc_endparse(mdoc));
	if (man)
		return(man_endparse(man));

	warnx("%s: not a manual", curp->file);
	return(0);
}


static int
pset(const char *buf, size_t pos, struct curparse *curp,
		struct man **man, struct mdoc **mdoc)
{

	/*
	 * Try to intuit which kind of manual parser should be used.  If
	 * passed in by command-line (-man, -mdoc), then use that
	 * explicitly.  If passed as -mandoc, then try to guess from the
	 * line: either skip comments, use -mdoc when finding `.Dt', or
	 * default to -man, which is more lenient.
	 */

	if (pos >= 3 && 0 == memcmp(buf, ".\\\"", 3))
		return(1);

	switch (curp->inttype) {
	case (INTT_MDOC):
		if (NULL == curp->mdoc) 
			curp->mdoc = mdoc_init(curp);
		if (NULL == (*mdoc = curp->mdoc))
			return(0);
		warnx("inheriting -mdoc parser");
		return(1);
	case (INTT_MAN):
		if (NULL == curp->man) 
			curp->man = man_init(curp);
		if (NULL == (*man = curp->man))
			return(0);
		warnx("inheriting -man parser");
		return(1);
	default:
		break;
	}

	if (pos >= 3 && 0 == memcmp(buf, ".Dd", 3))  {
		if (NULL == curp->mdoc) 
			curp->mdoc = mdoc_init(curp);
		if (NULL == (*mdoc = curp->mdoc))
			return(0);
		return(1);
	} 

	if (NULL == curp->man) 
		curp->man = man_init(curp);
	if (NULL == (*man = curp->man))
		return(0);
	return(1);
}


static int
moptions(enum intt *tflags, char *arg)
{

	if (0 == strcmp(arg, "doc"))
		*tflags = INTT_MDOC;
	else if (0 == strcmp(arg, "andoc"))
		*tflags = INTT_AUTO;
	else if (0 == strcmp(arg, "an"))
		*tflags = INTT_MAN;
	else {
		warnx("bad argument: -m%s", arg);
		return(0);
	}

	return(1);
}


static int
toptions(enum outt *tflags, char *arg)
{

	if (0 == strcmp(arg, "ascii"))
		*tflags = OUTT_ASCII;
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
	char		*toks[5];

	toks[0] = "ign-scope";
	toks[1] = "ign-escape";
	toks[2] = "ign-macro";
	toks[4] = "no-ign-macro";
	toks[5] = NULL;

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			*fflags |= IGN_SCOPE;
			break;
		case (1):
			*fflags |= IGN_ESCAPE;
			break;
		case (2):
			*fflags |= IGN_MACRO;
			break;
		case (3):
			*fflags |= NO_IGN_MACRO;
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
mdocwarn(void *arg, int line, int col, 
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


static int
manwarn(void *arg, int line, int col, const char *msg)
{
	struct curparse *curp;

	curp = (struct curparse *)arg;

	if ( ! (curp->wflags & WARN_WSYNTAX))
		return(1);

	warnx("%s:%d: syntax warning: %s (column %d)", 
			curp->file, line, msg, col);

	if ( ! (curp->wflags & WARN_WERR))
		return(1);

	warnx("%s: considering warnings as errors", 
			__progname);
	return(0);
}
