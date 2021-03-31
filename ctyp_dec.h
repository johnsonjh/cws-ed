/* do not change the 'unsigned char' to 'Uchar'. regex uses this, and it doesn't include opsys.h */
#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define _P	020
#define _C	040
#define _X	0100
#define	_B	0200
extern unsigned char _my_ctype_[];
extern unsigned char _my_tolower_[];
extern unsigned char _my_toupper_[];
extern unsigned char _my_tooppos_[];
#define	isalpha(c)	((_my_ctype_[(unsigned char)(c)])&(_U|_L))
#define	isupper(c)	((_my_ctype_[(unsigned char)(c)])&_U)
#define	islower(c)	((_my_ctype_[(unsigned char)(c)])&_L)
#define	isdigit(c)	((_my_ctype_[(unsigned char)(c)])&_N)
#define	isxdigit(c)	((_my_ctype_[(unsigned char)(c)])&_X)
#define	isspace(c)	((_my_ctype_[(unsigned char)(c)])&_S)
#define ispunct(c)	((_my_ctype_[(unsigned char)(c)])&_P)
#define isalnum(c)	((_my_ctype_[(unsigned char)(c)])&(_U|_L|_N))
#define isprint(c)	((_my_ctype_[(unsigned char)(c)])&(_P|_U|_L|_N|_B))
#define	isgraph(c)	((_my_ctype_[(unsigned char)(c)])&(_P|_U|_L|_N))
#define iscntrl(c)	((_my_ctype_[(unsigned char)(c)])&_C)
#define isascii(c)	((unsigned char)(c)<=0177)
#define toascii(c)	((c)&0177)
#define tolower(c)  (_my_tolower_[(unsigned char)(c)])
#define toupper(c)  (_my_toupper_[(unsigned char)(c)])
#define tooppos(c)  (_my_tooppos_[(unsigned char)(c)])
