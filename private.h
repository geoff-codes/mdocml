/* $Id: private.h,v 1.37 2008/12/10 12:05:33 kristaps Exp $ */
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
#ifndef PRIVATE_H
#define PRIVATE_H

#include <stdio.h>
#include <time.h>

#include "libmdocml.h"

struct	md_rbuf {
	int		 fd;		/* Open descriptor. */
	char		*name;		/* Name of file. */
	char		*buf;		/* Buffer. */
	size_t		 bufsz;		/* Size of buffer. */
	size_t		 line;		/* Current line number. */
#define	MD_LINE	 	(BUFSIZ)
	char		 linebuf[MD_LINE];
};

struct	md_mbuf {
	int		 fd;		/* Open descriptor. */
	char		*name;		/* Name of file. */
	char		*buf;		/* Buffer. */
	size_t		 bufsz;		/* Size of buffer. */
	size_t		 pos;		/* Position in buffer. */
};

#define	ROFFTok_Sp_A	 0
#define	ROFFTok_Sp_B	 1
#define	ROFFTok_Sp_F	 2
#define	ROFFTok_Sp_N	 3
#define	ROFFTok_Sp_R	 4
#define	ROFFTok_Sp_T	 5
#define	ROFFTok_Sp_V	 6
#define	ROFFTok_Space	 7
#define	ROFFTok_Null	 8
#define	ROFFTok_Hyphen	 9
#define	ROFFTok_Em	 10
#define	ROFFTok_En	 11
#define	ROFFTok_Ge	 12
#define	ROFFTok_Le	 13
#define	ROFFTok_Rquote	 14
#define	ROFFTok_Lquote	 15
#define	ROFFTok_Uparrow	 16
#define	ROFFTok_Acute	 17
#define	ROFFTok_Grave	 18
#define	ROFFTok_Pi	 19
#define	ROFFTok_Ne	 20
#define	ROFFTok_Lt	 21
#define	ROFFTok_Gt	 22
#define	ROFFTok_Plusmin	 23
#define	ROFFTok_Infty	 24
#define	ROFFTok_Bar	 25
#define	ROFFTok_Nan	 26
#define	ROFFTok_Quote	 27
#define	ROFFTok_Sp_0	 28
#define	ROFFTok_Slash	 29
#define	ROFFTok_MAX	 30

#define	ROFF___	 	 0
#define	ROFF_Dd		 1
#define	ROFF_Dt		 2
#define	ROFF_Os		 3
#define	ROFF_Sh		 4
#define	ROFF_Ss		 5
#define	ROFF_Pp		 6
#define	ROFF_D1		 7
#define	ROFF_Dl		 8
#define	ROFF_Bd		 9
#define	ROFF_Ed		 10
#define	ROFF_Bl		 11
#define	ROFF_El		 12
#define	ROFF_It		 13
#define	ROFF_Ad		 14
#define	ROFF_An		 15
#define	ROFF_Ar		 16
#define	ROFF_Cd		 17
#define	ROFF_Cm		 18
#define	ROFF_Dv		 19
#define	ROFF_Er		 20
#define	ROFF_Ev		 21
#define	ROFF_Ex		 22
#define	ROFF_Fa		 23
#define	ROFF_Fd		 24
#define	ROFF_Fl		 25
#define	ROFF_Fn		 26
#define	ROFF_Ft		 27
#define	ROFF_Ic		 28
#define	ROFF_In		 29
#define	ROFF_Li		 30
#define	ROFF_Nd		 31
#define	ROFF_Nm		 32
#define	ROFF_Op		 33
#define	ROFF_Ot		 34
#define	ROFF_Pa		 35
#define	ROFF_Rv		 36
#define	ROFF_St		 37
#define	ROFF_Va		 38
#define	ROFF_Vt		 39
#define	ROFF_Xr		 40
#define	ROFF__A		 41
#define	ROFF__B		 42
#define	ROFF__D		 43
#define	ROFF__I		 44
#define	ROFF__J		 45
#define	ROFF__N		 46
#define	ROFF__O		 47
#define	ROFF__P		 48
#define	ROFF__R		 49
#define	ROFF__T		 50
#define	ROFF__V		 51
#define ROFF_Ac		 52
#define ROFF_Ao		 53
#define ROFF_Aq		 54
#define ROFF_At		 55
#define ROFF_Bc		 56
#define ROFF_Bf		 57
#define ROFF_Bo		 58
#define ROFF_Bq		 59
#define ROFF_Bsx	 60
#define ROFF_Bx		 61
#define ROFF_Db		 62
#define ROFF_Dc		 63
#define ROFF_Do		 64
#define ROFF_Dq		 65
#define ROFF_Ec		 66
#define ROFF_Ef		 67
#define ROFF_Em		 68
#define ROFF_Eo		 69
#define ROFF_Fx		 70
#define ROFF_Ms		 71
#define ROFF_No		 72
#define ROFF_Ns		 73
#define ROFF_Nx		 74
#define ROFF_Ox		 75
#define ROFF_Pc		 76
#define ROFF_Pf		 77
#define ROFF_Po		 78
#define ROFF_Pq		 79
#define ROFF_Qc		 80
#define ROFF_Ql		 81
#define ROFF_Qo		 82
#define ROFF_Qq		 83
#define ROFF_Re		 84
#define ROFF_Rs		 85
#define ROFF_Sc		 86
#define ROFF_So		 87
#define ROFF_Sq		 88
#define ROFF_Sm		 89
#define ROFF_Sx		 90
#define ROFF_Sy		 91
#define ROFF_Tn		 92
#define ROFF_Ux		 93
#define ROFF_Xc		 94
#define ROFF_Xo		 95
#define	ROFF_Fo		 96
#define	ROFF_Fc		 97
#define	ROFF_Oo		 98
#define	ROFF_Oc		 99
#define	ROFF_Bk		 100
#define	ROFF_Ek		 101
#define	ROFF_Bt		 102
#define	ROFF_Hf		 103
#define	ROFF_Fr		 104
#define	ROFF_Ud		 105
#define	ROFF_MAX	 106

#define	ROFF_Split	 0
#define	ROFF_Nosplit	 1
#define	ROFF_Ragged	 2
#define	ROFF_Unfilled	 3
#define	ROFF_Literal	 4
#define	ROFF_File	 5
#define	ROFF_Offset	 6
#define	ROFF_Bullet	 7
#define	ROFF_Dash	 8
#define	ROFF_Hyphen	 9
#define	ROFF_Item	 10
#define	ROFF_Enum	 11
#define	ROFF_Tag	 12
#define	ROFF_Diag	 13
#define	ROFF_Hang	 14
#define	ROFF_Ohang	 15
#define	ROFF_Inset	 16
#define	ROFF_Column	 17
#define	ROFF_Width	 18
#define	ROFF_Compact	 19
#define	ROFF_Std	 20
#define ROFF_p1003_1_88	 21
#define ROFF_p1003_1_90	 22
#define ROFF_p1003_1_96	 23
#define ROFF_p1003_1_2001 24
#define ROFF_p1003_1_2004 25
#define ROFF_p1003_1	 26
#define ROFF_p1003_1b	 27
#define ROFF_p1003_1b_93 28
#define ROFF_p1003_1c_95 29
#define ROFF_p1003_1g_2000 30
#define ROFF_p1003_2_92	 31
#define ROFF_p1387_2_95	 32
#define ROFF_p1003_2	 33
#define ROFF_p1387_2	 34
#define ROFF_isoC_90	 35
#define ROFF_isoC_amd1	 36
#define ROFF_isoC_tcor1	 37
#define ROFF_isoC_tcor2	 38
#define ROFF_isoC_99	 39
#define ROFF_ansiC	 40
#define ROFF_ansiC_89	 41
#define ROFF_ansiC_99	 42
#define ROFF_ieee754	 43
#define ROFF_iso8802_3	 44
#define ROFF_xpg3	 45
#define ROFF_xpg4	 46
#define ROFF_xpg4_2	 47
#define ROFF_xpg4_3	 48
#define ROFF_xbd5	 49
#define ROFF_xcu5	 50
#define ROFF_xsh5	 51
#define ROFF_xns5	 52
#define ROFF_xns5_2d2_0	 53
#define ROFF_xcurses4_2	 54
#define ROFF_susv2	 55
#define ROFF_susv3	 56
#define ROFF_svid4	 57
#define	ROFF_Filled	 58
#define	ROFF_Words	 59
#define	ROFF_ARGMAX	 60

#define	ROFF_MAXLINEARG	 32

extern	const char *const *toknames;
extern	const char *const *tokargnames;

enum	roffmsg { 
	ROFF_WARN, 
	ROFF_ERROR
};

enum	roffmsec {
	ROFF_MSEC_1,
	ROFF_MSEC_2,
	ROFF_MSEC_3,
	ROFF_MSEC_3p,
	ROFF_MSEC_4,
	ROFF_MSEC_5,
	ROFF_MSEC_6,
	ROFF_MSEC_7,
	ROFF_MSEC_8,
	ROFF_MSEC_9,
	ROFF_MSEC_UNASS,
	ROFF_MSEC_DRAFT,
	ROFF_MSEC_PAPER,
	ROFF_MSEC_MAX
};

enum	roffatt {
	ROFF_ATT_V1,
	ROFF_ATT_V2,
	ROFF_ATT_V3,
	ROFF_ATT_V6,
	ROFF_ATT_V7,
	ROFF_ATT_32V,
	ROFF_ATT_V_1,
	ROFF_ATT_V_4,
	ROFF_ATT_MAX
};

enum	roffvol {
	ROFF_VOL_NONE,
	ROFF_VOL_AMD,
	ROFF_VOL_IND,
	ROFF_VOL_KM,
	ROFF_VOL_LOCAL,
	ROFF_VOL_PRM,
	ROFF_VOL_PS1,
	ROFF_VOL_SMM,
	ROFF_VOL_URM,
	ROFF_VOL_USD,
#define	ROFF_ARCH_START	ROFF_ARCH_ALPHA
	ROFF_ARCH_ALPHA,
	ROFF_ARCH_AMD64,
	ROFF_ARCH_AMIGA,
	ROFF_ARCH_ARC,
	ROFF_ARCH_ARMISH,
	ROFF_ARCH_AVIION,
	ROFF_ARCH_HP300,
	ROFF_ARCH_HPPA,
	ROFF_ARCH_HPPA64,
	ROFF_ARCH_I386,
	ROFF_ARCH_LANDISK,
	ROFF_ARCH_LUNA88K,
	ROFF_ARCH_MAC68K,
	ROFF_ARCH_MACPPC,
	ROFF_ARCH_MVME68K,
	ROFF_ARCH_MVME88K,
	ROFF_ARCH_MVMEPPC,
	ROFF_ARCH_PMAX,
	ROFF_ARCH_SGI,
	ROFF_ARCH_SPARC,
	ROFF_ARCH_SPARC64,
	ROFF_ARCH_SUN3,
	ROFF_ARCH_VAX,
	ROFF_ARCH_ZAURUS,
	ROFF_VOL_MAX
};

#define	ROFFSec_NMASK	(0x07)

#define	ROFFSec_NAME	(1 << 0)
#define	ROFFSec_SYNOP	(1 << 1)
#define	ROFFSec_DESC	(1 << 2)
#define	ROFFSec_RETVAL	(1 << 3)
#define	ROFFSec_ENV	(1 << 4)
#define	ROFFSec_FILES	(1 << 5)
#define	ROFFSec_EX	(1 << 6)
#define	ROFFSec_DIAG	(1 << 7)
#define	ROFFSec_ERRS	(1 << 8)
#define	ROFFSec_SEEALSO	(1 << 9)
#define	ROFFSec_STAND	(1 << 10)
#define	ROFFSec_HIST	(1 << 11)
#define	ROFFSec_AUTH	(1 << 12)
#define	ROFFSec_CAVEATS	(1 << 13)
#define	ROFFSec_BUGS	(1 << 14)
#define	ROFFSec_OTHER	(1 << 15)

struct	roffcb {
	int	(*roffmsg)(void *, enum roffmsg, 
			const char *, const char *, const char *);
	int	(*roffhead)(void *, const struct tm *, const char *, 
			const char *, enum roffmsec, enum roffvol);
	int	(*rofftail)(void *, const struct tm *, const char *, 
			const char *, enum roffmsec, enum roffvol);
	int	(*roffdata)(void *, int, const char *, const char *);
	int	(*roffin)(void *, int, int *, const char **);
	int	(*roffout)(void *, int);
	int	(*roffblkin)(void *, int, int *, const char **);
	int	(*roffblkout)(void *, int);
	int	(*roffblkheadin)(void *, int, int *, const char **);
	int	(*roffblkheadout)(void *, int);
	int	(*roffblkbodyin)(void *, int, int *, const char **);
	int	(*roffblkbodyout)(void *, int);
	int	(*roffspecial)(void *, int, const char *, 
			const int *, const char **, const char **);
};

struct	rofftree;

__BEGIN_DECLS

typedef	void  (*(*md_init)(const struct md_args *, 
			struct md_mbuf *, const struct md_rbuf *));
typedef	int	(*md_line)(void *, char *);
typedef	int	(*md_exit)(void *, int);
void		 *md_init_html(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *);
int		  md_line_html(void *, char *);
int		  md_exit_html(void *, int);
void		 *md_init_xml(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *);
int		  md_line_xml(void *, char *);
int		  md_exit_xml(void *, int);
int	 	  md_buf_puts(struct md_mbuf *, const char *, size_t);
int	 	  md_buf_putchar(struct md_mbuf *, char);
int	 	  md_buf_putstring(struct md_mbuf *, const char *);
struct	rofftree *roff_alloc(const struct roffcb *, void *);
int		  roff_engine(struct rofftree *, char *);
int		  roff_free(struct rofftree *, int);
int		  rofftok_scan(const char *, int *);
char		 *roff_literal(int, const int *,
			const char **, const char **);
char		 *roff_fmtstring(int);
char		 *roff_msecname(enum roffmsec);
enum roffmsec	  roff_msec(const char *);
int		  roff_sec(const char **);
enum roffatt	  roff_att(const char *);
enum roffvol	  roff_vol(const char *);
char		 *roff_volname(enum roffvol);

__END_DECLS

#endif /*!PRIVATE_H*/
