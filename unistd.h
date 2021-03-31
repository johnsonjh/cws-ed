#ifdef _AIX
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef VMS
#include <unixio.h>
#endif

#ifdef sparc
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef __osf__
#include <unistd.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifdef GNUOS2	/* don't change the order */
#include <unistd.h>
#else
#ifdef WIN32
#include <io.h>			/* for open, read, write, close */
#include <direct.h>		/* for chdir, etc */
#else
#ifdef GNUDOS
#include <std.h>
#endif
#endif
#endif
