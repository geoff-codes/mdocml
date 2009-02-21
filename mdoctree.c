	/* $Id: mdocml.c,v 1.54 2009/02/21 15:34:46 kristaps Exp $ */
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
#include <sys/stat.h>
#include <sys/param.h>

#include <assert.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mdoc.h"

#define	MD_LINE_SZ	(256)		/* Max input line size. */

struct	md_parse {
	int		  warn;		/* Warning flags. */
#define	MD_WARN_SYNTAX	 (1 << 0)	/* Show syntax warnings. */
#define	MD_WARN_COMPAT	 (1 << 1)	/* Show compat warnings. */
#define	MD_WARN_ALL	 (0x03)		/* Show all warnings. */
#define	MD_WARN_ERR	 (1 << 2)	/* Make warnings->errors. */
	int		  dbg;		/* Debug level. */
	struct mdoc	 *mdoc;		/* Active parser. */
	char		 *buf;		/* Input buffer. */
	u_long		  bufsz;	/* Input buffer size. */
	char		 *in;		/* Input file name. */
	int		  fdin;		/* Input file desc. */
};

extern	char	 	 *__progname;

static	void		  usage(void);
static	int		  getsopts(struct md_parse *, char *);
static	int		  parse(struct md_parse *);
static	void		  msg_msg(void *, int, int, const char *);
static	int		  msg_err(void *, int, int, const char *);
static	int		  msg_warn(void *, int, int, 
				enum mdoc_warn, const char *);

extern	void		  treeprint(const struct mdoc_node *,
				const struct mdoc_meta *);

#ifdef __linux__
extern	int		  getsubopt(char **, char *const *, char **);
#endif

int
main(int argc, char *argv[])
{
	struct md_parse	 p;
	struct mdoc_cb	 cb;
	struct stat	 st;
	int		 c;
	extern char	*optarg;
	extern int	 optind;

	(void)memset(&p, 0, sizeof(struct md_parse));

	while (-1 != (c = getopt(argc, argv, "vW:")))
		switch (c) {
		case ('v'):
			p.dbg++;
			break;
		case ('W'):
			if ( ! getsopts(&p, optarg))
				return(0);
			break;
		default:
			usage();
			return(0);
		}

	argv += optind;
	argc -= optind;

	/* Initialise the input file. */

	p.in = "-";
	p.fdin = STDIN_FILENO;

	if (argc > 0) {
		p.in = *argv++;
		p.fdin = open(p.in, O_RDONLY, 0);
		if (-1 == p.fdin)
			err(1, "%s", p.in);
	}

	/* Allocate a buffer to be BUFSIZ/block size. */

	if (-1 == fstat(p.fdin, &st)) {
		warn("%s", p.in);
		p.bufsz = BUFSIZ;
	} else 
		p.bufsz = MAX(st.st_blksize, BUFSIZ);

	p.buf = malloc(p.bufsz);
	if (NULL == p.buf)
		err(1, "malloc");

	/* Allocate the parser. */

	cb.mdoc_err = msg_err;
	cb.mdoc_warn = msg_warn;
	cb.mdoc_msg = msg_msg;

	p.mdoc = mdoc_alloc(&p, &cb);

	/* Parse the input file. */

	c = parse(&p);
	free(p.buf);

	if (STDIN_FILENO != p.fdin && -1 == close(p.fdin))
		warn("%s", p.in);

	if (0 == c) {
		mdoc_free(p.mdoc);
		return(EXIT_FAILURE);
	}

	/* If the parse succeeded, print it out. */

	treeprint(mdoc_node(p.mdoc), mdoc_meta(p.mdoc));
	mdoc_free(p.mdoc);

	return(EXIT_SUCCESS);
}


static int
getsopts(struct md_parse *p, char *arg)
{
	char		*v;
	char		*toks[] = { "all", "compat", 
				"syntax", "error", NULL };

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			p->warn |= MD_WARN_ALL;
			break;
		case (1):
			p->warn |= MD_WARN_COMPAT;
			break;
		case (2):
			p->warn |= MD_WARN_SYNTAX;
			break;
		case (3):
			p->warn |= MD_WARN_ERR;
			break;
		default:
			usage();
			return(0);
		}

	return(1);
}


static int
parse(struct md_parse *p)
{
	ssize_t		 sz, i;
	size_t		 pos;
	char		 line[MD_LINE_SZ];
	int		 lnn;

	/*
	 * This is a little more complicated than fgets.  TODO: have
	 * some benchmarks that show it's faster (note that I want to
	 * check many, many manuals simultaneously, so speed is
	 * important).  Fill a buffer (sized to the block size) with a
	 * single read, then parse \n-terminated lines into a line
	 * buffer, which is passed to the parser.  Hard-code the line
	 * buffer to a particular size -- a reasonable assumption.
	 */

	for (lnn = 1, pos = 0; ; ) {
		if (-1 == (sz = read(p->fdin, p->buf, p->bufsz))) {
			warn("%s", p->in);
			return(0);
		} else if (0 == sz) 
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' != p->buf[i]) {
				if (pos < sizeof(line)) {
					line[(int)pos++] = p->buf[(int)i];
					continue;
				}
				warnx("%s: line %d too long", p->in, lnn);
				return(0);
			}
	
			line[(int)pos] = 0;
			if ( ! mdoc_parseln(p->mdoc, lnn, line))
				return(0);

			lnn++;
			pos = 0;
		}
	}

	return(mdoc_endparse(p->mdoc));
}


static int
msg_err(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	warnx("%s:%d: error: %s (column %d)", 
			p->in, line, msg, col);
	return(0);
}


static void
msg_msg(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	if (0 == p->dbg)
		return;

	warnx("%s:%d: debug: %s (column %d)", 
			p->in, line, msg, col);
}


static int
msg_warn(void *arg, int line, int col, 
		enum mdoc_warn type, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	switch (type) {
	case (WARN_COMPAT):
		if (p->warn & MD_WARN_COMPAT)
			break;
		return(1);
	case (WARN_SYNTAX):
		if (p->warn & MD_WARN_SYNTAX)
			break;
		return(1);
	}

	warnx("%s:%d: warning: %s (column %d)", 
			p->in, line, msg, col);

	if ( ! (p->warn & MD_WARN_ERR))
		return(1);

	warnx("%s: considering warnings as errors", __progname);
	return(0);
}


static void
usage(void)
{

	warnx("usage: %s [-v] [-Wwarn...] [infile]", __progname);
}

