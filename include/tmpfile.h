#ifndef LIB_TEMPORARY_FILE_H
#define LIB_TEMPORARY_FILE_H

#ifdef _WIN32
/* Load size_t on windows */
#include <crtdefs.h>
#else

#ifndef _GNU_SOURCE
// In the case that are compiling on linux, we need to define _GNU_SOURCE
// *before* randombytes.h is included. Otherwise SYS_getrandom will not be
// declared.
#if defined(__linux__) || defined(__GNU__)
# define _GNU_SOURCE
#endif /* defined(__linux__) || defined(__GNU__) */
#endif

#include <unistd.h>
#endif /* _WIN32 */

#include <limits.h> // CHAR_BIT

// sanity check
#if CHAR_BIT != 8
#error system does not support 8 bit addressing
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

class TempFile {
    #ifdef _WIN32
    HANDLE fd;
    const HANDLE invalid_fd = INVALID_HANDLE_VALUE;
    #else
    int fd;
    const int invalid_fd = -1;
    #endif

    char * path;

public:

    bool is_handle_valid();

    TempFile();
    TempFile(const char * template_prefix);
    bool construct(const char * template_prefix);
    const char * get_path() const;
    #ifdef _WIN32
    HANDLE get_handle() const;
    #else
    int get_handle() const;
    #endif
    ~TempFile();
};

#endif