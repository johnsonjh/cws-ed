/* Included by: all routines */
#include "config.h"
#ifdef GNUOS2
#define GNUDOS
#endif

#ifdef NeXT
#define NO_SBRK
#define CUSERID_ENV "USER"
#endif

#ifdef __bsdi__
#define CUSERID_ENV "USER"
#endif

#ifdef __osf__
#define NO_SBRK
#endif

#ifdef USE_NCS
#define NONPRINTNCS(c) (c & 0x7f) < 32 || (c & 0x7f) > 126
#else
#define NONPRINTNCS(c) c < 32 || c > 126
#endif

typedef int Int;
typedef long Long;
#ifdef NO_SIGNED_CHAR
typedef char Schar;
#else
typedef signed char Schar;
#endif
typedef char Char;
typedef unsigned char Uchar;

extern void *imalloc();
