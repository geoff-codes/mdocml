/*	$Id: mandocdb.c,v 1.46 2012/03/23 06:52:17 kristaps Exp $ */
/*
 * Copyright (c) 2012 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <sys/param.h>

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "manpath.h"
#include "mansearch.h"

int
main(int argc, char *argv[])
{
	int		 ch;
	size_t		 i, sz;
	struct manpage	*res;
	char		*conf_file, *defpaths, *auxpaths,
			*arch, *sec;
	struct manpaths	 paths;
	char		*progname;
	extern char	*optarg;
	extern int	 optind;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	auxpaths = defpaths = conf_file = arch = sec = NULL;
	memset(&paths, 0, sizeof(struct manpaths));

	while (-1 != (ch = getopt(argc, argv, "C:M:m:S:s:")))
		switch (ch) {
		case ('C'):
			conf_file = optarg;
			break;
		case ('M'):
			defpaths = optarg;
			break;
		case ('m'):
			auxpaths = optarg;
			break;
		case ('S'):
			arch = optarg;
			break;
		case ('s'):
			sec = optarg;
			break;
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	if (0 == argc)
		goto usage;

	manpath_parse(&paths, conf_file, defpaths, auxpaths);
	ch = mansearch(&paths, arch, sec, argc, argv, &res, &sz);
	manpath_free(&paths);

	if (0 == ch)
		goto usage;

	for (i = 0; i < sz; i++) {
		printf("%s - %s\n", res[i].file, res[i].desc);
		free(res[i].desc);
	}

	free(res);
	return(sz ? EXIT_SUCCESS : EXIT_FAILURE);
usage:
	fprintf(stderr, "usage: %s [-C conf] "
			 	  "[-M paths] "
				  "[-m paths] "
				  "[-S arch] "
				  "[-s section] "
			          "expr ...\n", 
				  progname);
	return(EXIT_FAILURE);
}
