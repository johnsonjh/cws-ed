#ifdef VMS
#include <file.h>
#include <unixio.h>
#include <stat.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(x) (x & S_IFDIR)
#endif

#ifdef NeXT
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif
