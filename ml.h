/* $Id: ml.h,v 1.7 2008/12/05 19:45:15 kristaps Exp $ */
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
#ifndef ML_H
#define ML_H

struct	md_mlg;

enum	md_ns {
	MD_NS_BLOCK,
	MD_NS_HEAD,
	MD_NS_BODY,
	MD_NS_INLINE,
	MD_NS_DEFAULT,
};

struct	ml_cbs {
	int	(*ml_begin)(struct md_mbuf *, 
			const struct md_args *,
			const struct tm *, 
			const char *, const char *,
			const char *, const char *);
	int	(*ml_end)(struct md_mbuf *, 
			const struct md_args *);
	ssize_t	(*ml_beginstring)(struct md_mbuf *,
			const struct md_args *,
			const char *, size_t);
	ssize_t	(*ml_endstring)(struct md_mbuf *,
			const struct md_args *,
			const char *, size_t);
	ssize_t	(*ml_endtag)(struct md_mbuf *, 
			void *, const struct md_args *, 
			enum md_ns, int);
	ssize_t	(*ml_begintag)(struct md_mbuf *, 
			void *, const struct md_args *, 
			enum md_ns, int, 
			const int *, const char **);
	int	(*ml_alloc)(void **);
	void	(*ml_free)(void *);
};

__BEGIN_DECLS

int		  ml_putstring(struct md_mbuf *, 
			const char *, size_t *);
int		  ml_nputstring(struct md_mbuf *, 
			const char *, size_t, size_t *);
int		  ml_nputs(struct md_mbuf *, 
			const char *, size_t, size_t *);
int		  ml_puts(struct md_mbuf *, const char *, size_t *);
int		  ml_putchars(struct md_mbuf *, 
			char, size_t, size_t *);

struct md_mlg	 *mlg_alloc(const struct md_args *, 
			const struct md_rbuf *, struct md_mbuf *,
			const struct ml_cbs *);
int		  mlg_exit(struct md_mlg *, int);
int		  mlg_line(struct md_mlg *, char *);

__END_DECLS

#endif /*!ML_H*/
