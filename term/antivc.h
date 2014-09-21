#ifndef ANTIVC
#define ANTIVC

#ifdef _MSC_VER

#include <io.h>

#ifndef PATH_MAX
#define PATH_MAX MAXPATHLEN
#endif

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define S_ISDIR(mode)  (((mode) & S_IFMT) == (S_IFDIR)) 
#define S_ISREG(mode)  (((mode) & S_IFMT) == (S_IFREG)) 

#define read _read
#define open _open
#define close _close
#define write _write

#endif

#endif
