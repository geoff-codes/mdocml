/*	$Id: manup.c,v 1.1 2011/11/24 10:33:38 kristaps Exp $ */
/*
 * Copyright (c) 2011 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
# include <db_185.h>
#else
# include <db.h>
#endif

#include "manpath.h"

#define	xstrlcpy(_dst, _src, _sz) \
	do if (strlcpy((_dst), (_src), (_sz)) >= (_sz)) { \
		fprintf(stderr, "%s: Path too long", (_dst)); \
		exit(EXIT_FAILURE); \
	} while (/* CONSTCOND */0)

#define	xstrlcat(_dst, _src, _sz) \
	do if (strlcat((_dst), (_src), (_sz)) >= (_sz)) { \
		fprintf(stderr, "%s: Path too long", (_dst)); \
		exit(EXIT_FAILURE); \
	} while (/* CONSTCOND */0)

static	int		 indexhtml(char *);
static	int		 jobstart(const char *, const char *, pid_t *);
static	int		 jobwait(pid_t);
static	int		 manup(const struct manpaths *, const char *);
static	int		 mkpath(char *, mode_t, mode_t);
static	int		 treecpy(char *, char *);
static	int		 update(char *, char *);
static	void		 usage(void);

static	const char	*progname;
static	int		 verbose;
static	int		 force;

int
main(int argc, char *argv[])
{
	int		 ch;
	char		*aux, *base;
	const char	*dir;
	struct manpaths	 dirs;
	extern char	*optarg;
	extern int	 optind;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	aux = base = NULL;
	dir = "/var/www/cache/man.cgi";

	while (-1 != (ch = getopt(argc, argv, "fm:M:o:v")))
		switch (ch) {
		case ('f'):
			force = 1;
			break;
		case ('m'):
			aux = optarg;
			break;
		case ('M'):
			base = optarg;
			break;
		case ('o'):
			dir = optarg;
			break;
		case ('v'):
			verbose++;
			break;
		default:
			usage();
			return(EXIT_FAILURE);
		}

	argc -= optind;
	argv += optind;

	if (argc > 0) {
		usage();
		return(EXIT_FAILURE);
	}

	memset(&dirs, 0, sizeof(struct manpaths));
	manpath_parse(&dirs, base, aux);
	ch = manup(&dirs, dir);
	manpath_free(&dirs);
	return(ch ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
usage(void)
{
	
	fprintf(stderr, "usage: %s "
			"[-fv] "
			"[-o path] "
			"[-m manpath] "
			"[-M manpath]\n",
			progname);
}

/*
 * If "src" file doesn't exist (errors out), return -1.  Otherwise,
 * return 1 if "src" is newer (which also happens "dst" doesn't exist)
 * and 0 otherwise.
 */
static int
isnewer(const char *dst, const char *src)
{
	struct stat	 s1, s2;

	if (-1 == stat(src, &s1))
		return(-1);
	if (force)
		return(1);

	return(-1 == stat(dst, &s2) ? 1 : s1.st_mtime > s2.st_mtime);
}

/*
 * Copy the contents of one file into another.
 * Returns 0 on failure, 1 on success.
 */
static int
filecpy(const char *dst, const char *src)
{
	char		 buf[BUFSIZ];
	int		 sfd, dfd, rc;
	ssize_t		 rsz, wsz;

	sfd = dfd = -1;
	rc = 0;

	if (-1 == (dfd = open(dst, O_CREAT|O_TRUNC|O_WRONLY, 0644))) {
		perror(dst);
		goto out;
	} else if (-1 == (sfd = open(src, O_RDONLY, 0))) {
		perror(src);
		goto out;
	} 

	while ((rsz = read(sfd, buf, BUFSIZ)) > 0)
		if (-1 == (wsz = write(dfd, buf, (size_t)rsz))) {
			perror(dst);
			goto out;
		} else if (wsz < rsz) {
			fprintf(stderr, "%s: Short write\n", dst);
			goto out;
		}
	
	if (rsz < 0)
		perror(src);
	else
		rc = 1;
out:
	if (-1 != sfd)
		close(sfd);
	if (-1 != dfd)
		close(dfd);

	return(rc);
}

/*
 * Clean up existing child.
 * Return 1 if cleaned up fine (or none was started) and 0 otherwise.
 */
static int
jobwait(pid_t pid)
{
	int		 st;

	if (-1 == pid)
		return(1);

	if (-1 == waitpid(pid, &st, 0)) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	return(WIFEXITED(st) && 0 == WEXITSTATUS(st));
}

/*
 * Start a job (child process), first making sure that the prior one has
 * finished.
 * Return 1 if the prior child exited and the new one started, else 0.
 */
static int
jobstart(const char *dst, const char *src, pid_t *pid)
{
	int		 fd;

	if ( ! jobwait(*pid))
		return(0);

	if (-1 == (*pid = fork())) {
		perror(NULL);
		exit(EXIT_FAILURE);
	} else if (*pid > 0)
		return(1);

	if (-1 == (fd = open(dst, O_WRONLY|O_TRUNC|O_CREAT, 0644))) {
		perror(dst);
		exit(EXIT_FAILURE);
	}

	if (-1 == dup2(fd, STDOUT_FILENO)) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	execlp("mandoc", "mandoc", "-T", "html", 
			"-O", "fragment",
			"-O", "man=man.cgi?expr=%N&sec=%S", 
			src, (char *)NULL);

	perror("mandoc");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

/*
 * Pass over the recno database and re-create HTML pages if they're
 * found to be out of date.
 * Returns -1 on fatal error, 1 on success.
 */
static int
indexhtml(char *dst)
{
	DB		*db;
	DBT		 key, val;
	size_t		 sz;
	int		 c, rc;
	unsigned int	 fl;
	const char	*f;
	char		*d;
	char		 fname[MAXPATHLEN];
	pid_t		 pid;

	sz = strlen(dst);
	pid = -1;

	xstrlcpy(fname, dst, MAXPATHLEN);
	xstrlcat(fname, "/mandoc.index", MAXPATHLEN);

	db = dbopen(fname, O_RDONLY, 0, DB_RECNO, NULL);
	if (NULL == db) {
		perror(fname);
		return(-1);
	}

	fl = R_FIRST;
	while (0 == (c = (*db->seq)(db, &key, &val, fl))) {
		fl = R_NEXT;
		f = (const char *)val.data;

		dst[(int)sz] = '\0';

		xstrlcat(dst, "/", MAXPATHLEN);
		xstrlcat(dst, f, MAXPATHLEN);
		xstrlcat(dst, ".html", MAXPATHLEN);

		if (-1 == (rc = isnewer(dst, f))) {
			fprintf(stderr, "%s: Manpage missing\n", f);
			break;
		} else if (0 == rc)
			continue;

		d = strrchr(dst, '/');
		assert(NULL != d);
		*d = '\0';

		if (-1 == mkpath(dst, 0755, 0755)) {
			perror(dst);
			break;
		}

		*d = '/';
		if ( ! jobstart(dst, f, &pid))
			break;
		if (verbose)
			printf("%s\n", dst);
	}

	(*db->close)(db);

	if (c < 0)
		perror(fname);
	if ( ! jobwait(pid))
		c = -1;

	return(1 == c ? 1 : -1);
}

/*
 * Copy both recno and btree databases into the destination.
 * Call in to begin recreating HTML files.
 * Return -1 on fatal error and 1 if the update went well.
 */
static int
update(char *dst, char *src)
{
	size_t		 dsz, ssz;

	dsz = strlen(dst);
	ssz = strlen(src);

	xstrlcat(src, "/mandoc.db", MAXPATHLEN);
	xstrlcat(dst, "/mandoc.db", MAXPATHLEN);

	if ( ! filecpy(dst, src))
		return(-1);
	if (verbose)
		printf("%s\n", dst);

	dst[(int)dsz] = src[(int)ssz] = '\0';

	xstrlcat(src, "/mandoc.index", MAXPATHLEN);
	xstrlcat(dst, "/mandoc.index", MAXPATHLEN);

	if ( ! filecpy(dst, src))
		return(-1);
	if (verbose)
		printf("%s\n", dst);

	dst[(int)dsz] = '\0';

	return(indexhtml(dst));
}

/*
 * See if btree or recno databases in the destination are out of date
 * with respect to a single manpath component.
 * Return -1 on fatal error, 0 if the source is no longer valid (and
 * shouldn't be listed), and 1 if the update went well.
 */
static int
treecpy(char *dst, char *src)
{
	size_t		 dsz, ssz;
	int		 rc;

	dsz = strlen(dst);
	ssz = strlen(src);

	xstrlcat(src, "/mandoc.index", MAXPATHLEN);
	xstrlcat(dst, "/mandoc.index", MAXPATHLEN);

	if (-1 == (rc = isnewer(dst, src)))
		return(0);

	dst[(int)dsz] = src[(int)ssz] = '\0';

	if (1 == rc)
		return(update(dst, src));

	xstrlcat(src, "/mandoc.db", MAXPATHLEN);
	xstrlcat(dst, "/mandoc.db", MAXPATHLEN);

	if (-1 == (rc = isnewer(dst, src)))
		return(0);
	else if (rc == 0)
		return(1);

	dst[(int)dsz] = src[(int)ssz] = '\0';

	return(update(dst, src));
}

/*
 * Update the destination's file-tree with respect to changes in the
 * source manpath components.
 * "Change" is defined by an updated index or btree database.
 * Returns 1 on success, 0 on failure.
 */
static int
manup(const struct manpaths *dirs, const char *dir)
{
	char		 dst[MAXPATHLEN],
			 src[MAXPATHLEN];
	const char	*path;
	int		 i, c;
	size_t		 sz;
	FILE		*f;

	xstrlcpy(dst, dir, MAXPATHLEN);
	xstrlcat(dst, "/etc", MAXPATHLEN);

	if (-1 == mkpath(dst, 0755, 0755)) {
		perror(dst);
		return(0);
	}

	xstrlcat(dst, "/man.conf", MAXPATHLEN);

	if (verbose)
		printf("%s\n", dst);

	if (NULL == (f = fopen(dst, "w"))) {
		perror(dst);
		return(0);
	}

	xstrlcpy(dst, dir, MAXPATHLEN);
	sz = strlen(dst);

	for (i = 0; i < dirs->sz; i++) {
		path = dirs->paths[i];

		dst[(int)sz] = '\0';
		xstrlcat(dst, path, MAXPATHLEN);

		if (-1 == mkpath(dst, 0755, 0755)) {
			perror(dst);
			break;
		}

		xstrlcpy(src, path, MAXPATHLEN);

		if (-1 == (c = treecpy(dst, src)))
			break;
		else if (0 == c)
			continue;

		/*
		 * We want to use a relative path here because manpath.h
		 * will realpath() when invoked with man.cgi, and we'll
		 * make sure to chdir() into the cache directory before.
		 *
		 * This allows the cache directory to be in an arbitrary
		 * place, working in both chroot() and non-chroot()
		 * "safe" modes.
		 */
		assert('/' == path[0]);
		fprintf(f, "_whatdb %s/whatis.db\n", path + 1);
	}

	fclose(f);
	return(i == dirs->sz);
}

/*
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static int
mkpath(char *path, mode_t mode, mode_t dir_mode)
{
	struct stat sb;
	char *slash;
	int done, exists;

	slash = path;

	for (;;) {
		/* LINTED */
		slash += strspn(slash, "/");
		/* LINTED */
		slash += strcspn(slash, "/");

		done = (*slash == '\0');
		*slash = '\0';

		/* skip existing path components */
		exists = !stat(path, &sb);
		if (!done && exists && S_ISDIR(sb.st_mode)) {
			*slash = '/';
			continue;
		}

		if (mkdir(path, done ? mode : dir_mode) == 0) {
			if (mode > 0777 && chmod(path, mode) < 0)
				return (-1);
		} else {
			if (!exists) {
				/* Not there */
				return (-1);
			}
			if (!S_ISDIR(sb.st_mode)) {
				/* Is there, but isn't a directory */
				errno = ENOTDIR;
				return (-1);
			}
		}

		if (done)
			break;

		*slash = '/';
	}

	return (0);
}
