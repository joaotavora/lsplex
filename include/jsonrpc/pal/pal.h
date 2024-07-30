#if defined(_MSC_VER) || defined(__MINGW64__)
#include "jsonrpc/pal/pal_windows.h"
#else
#include "jsonrpc/pal/pal_posix.h"
#endif
