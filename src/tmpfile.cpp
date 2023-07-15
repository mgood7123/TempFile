#include <tmpfile.h>

/*
 * Write `n` bytes of high quality random bytes to `buf`
 */
int randombytes(void *buf, size_t n);

#ifdef _WIN32

#include <time.h>
#include <errno.h>

/* These are the characters used in temporary filenames.  */
static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define NUM_LETTERS (62)

int gettimeofday(struct timeval_64_t *spec)
{
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime      -=116444736000000000i64;  //1jan1601 to 1jan1970
    spec->tv_sec  =wintime / 10000000i64;  //seconds
    spec->tv_usec =wintime % 10000000i64;  //milli-seconds
    return 0;
}

static void brain_damaged_fillrand(unsigned char *buf, unsigned int len)
{
    unsigned int i, k;
    struct timeval_64_t tv;
    uint32_t high, low, rh;
    static uint64_t value;
    gettimeofday(&tv);
    value += ((uint64_t) tv.tv_usec << 16) ^ tv.tv_sec ^ GetCurrentProcessId();
    low = value & UINT32_MAX;
    high = value >> 32;
    for (i = 0; i < len; ++i) {
        rh = high % NUM_LETTERS;
        high /= NUM_LETTERS;
#define _L ((UINT32_MAX % NUM_LETTERS + 1) % NUM_LETTERS)
        k = (low % NUM_LETTERS) + (_L * rh);
#undef _L
#define _H ((UINT32_MAX / NUM_LETTERS) + ((UINT32_MAX % NUM_LETTERS + 1) / NUM_LETTERS))
        low = (low / NUM_LETTERS) + (_H * rh) + (k / NUM_LETTERS);
#undef _H
        k %= NUM_LETTERS;
        buf[i] = letters[k];
    }
}

using LP64__LONG = long long;

long long random(void) {
    LP64__LONG randomness[1];
    if (randombytes((unsigned char*)randomness, sizeof(LP64__LONG)) != sizeof(LP64__LONG)) {
        /* if random device nodes failed us, lets use the braindamaged ver */
        brain_damaged_fillrand((unsigned char*)randomness, sizeof(LP64__LONG));
    }
    return randomness[0];
}

#define MS_PER_SEC      1000ULL     // MS = milliseconds
#define US_PER_MS       1000ULL     // US = microseconds
#define HNS_PER_US      10ULL       // HNS = hundred-nanoseconds (e.g., 1 hns = 100 ns)
#define NS_PER_US       1000ULL

#define HNS_PER_SEC     (MS_PER_SEC * US_PER_MS * HNS_PER_US)
#define NS_PER_HNS      (100ULL)    // NS = nanoseconds
#define NS_PER_SEC      (MS_PER_SEC * US_PER_MS * NS_PER_US)

static int clock_gettime_monotonic(struct timespec_N_bit *tv)
{
    static LARGE_INTEGER ticksPerSec;
    LARGE_INTEGER ticks;
    double seconds;

    if (!ticksPerSec.QuadPart) {
        QueryPerformanceFrequency(&ticksPerSec);
        if (!ticksPerSec.QuadPart) {
            errno = ENOTSUP;
            return -1;
        }
    }

    QueryPerformanceCounter(&ticks);

    seconds = (double) ticks.QuadPart / (double) ticksPerSec.QuadPart;
    tv->tv_sec = (time_64_t) seconds;
    tv->tv_nsec = (LP64__LONG)((ULONGLONG)(seconds * NS_PER_SEC) % NS_PER_SEC);

    return 0;
}

static int clock_gettime_realtime(struct timespec_N_bit *tv)
{
    FILETIME ft;
    ULARGE_INTEGER hnsTime;

    GetSystemTimeAsFileTime(&ft);

    hnsTime.LowPart = ft.dwLowDateTime;
    hnsTime.HighPart = ft.dwHighDateTime;

    // To get POSIX Epoch as baseline, subtract the number of hns intervals from Jan 1, 1601 to Jan 1, 1970.
    hnsTime.QuadPart -= (11644473600ULL * HNS_PER_SEC);

    // modulus by hns intervals per second first, then convert to ns, as not to lose resolution
    tv->tv_nsec = (LP64__LONG) ((hnsTime.QuadPart % HNS_PER_SEC) * NS_PER_HNS);
    tv->tv_sec = (LP64__LONG) (hnsTime.QuadPart / HNS_PER_SEC);

    return 0;
}

int clock_gettime(clockid_t type, struct timespec_N_bit *tp)
{
    if (type == CLOCK_MONOTONIC)
    {
        return clock_gettime_monotonic(tp);
    }
    else if (type == CLOCK_REALTIME)
    {
        return clock_gettime_realtime(tp);
    }

    errno = ENOTSUP;
    return -1;
}

char * strndup (const char *s, size_t n)
{
    char *result;
    size_t len = strnlen (s, n);

    result = (char *) malloc (len + 1);
    if (!result)
    return 0;

    result[len] = '\0';
    return (char *) memcpy (result, s, len);
}

#else
#include <string.h> // strdup
#include <stdlib.h> // free
#endif

// based on
// https://github.com/wbx-github/uclibc-ng/blob/master/libc/misc/internals/tempname.c#L166

/* Generate a temporary file name based on TMPL. TMPL must match the
   rules for mk[s]temp[s] (i.e. end in "prefixXXXXXXsuffix"). The name
   constructed does not exist at the time of the call to __gen_tempname.
   TMPL is overwritten with the result.
   KIND may be one of:
   __GT_NOCREATE:       simply verify that the name does not exist
                        at the time of the call. mode argument is ignored.
   __GT_FILE:           create the file using open(O_CREAT|O_EXCL)
                        and return a read-write fd with given mode.
   __GT_BIGFILE:        same as __GT_FILE but use open64().
   __GT_DIR:            create a directory with given mode.
*/

bool TempFile::is_handle_valid() {
    return this->fd == this->invalid_fd;
}

TempFile::TempFile() {
    this->fd = this->invalid_fd;
    this->path = nullptr;
}

TempFile::TempFile(const char * template_prefix) {
    this->fd = this->invalid_fd;
    this->path = nullptr;

    std::string template_XXXXXX_str = std::string(template_prefix) + "XXXXXX";

#ifdef _WIN32

    char *XXXXXX;
    unsigned int i;
    int save_errno = errno;
    unsigned char randomness[6];
    size_t len;
    int suffixlen = 0; // always zero

    len = template_XXXXXX_str.length*();
    /* This is where the Xs start.  */
    XXXXXX = template_XXXXXX + len - 6 - suffixlen;
    if (len < 6 || suffixlen < 0 || suffixlen > len - 6
        || strncmp (XXXXXX, "XXXXXX", 6))
    {
        errno = EINVAL;
        this->fd = this->invalid_fd;
        return;
    }
    
#ifndef TMP_MAX
#define TMP_MAX 238328
#endif

    this->path = new char[MAX_PATH];
    char tmp_path[MAX_PATH];

    for (i = 0; i < TMP_MAX; ++i) {
        unsigned char j;
        /* Get some random data.  */
        if (randombytes(randomness, sizeof(randomness)) != sizeof(randomness)) {
            /* if random device nodes failed us, lets use the braindamaged ver */
            brain_damaged_fillrand(randomness, sizeof(randomness));
        }
        for (j = 0; j < sizeof(randomness); ++j)
            XXXXXX[j] = letters[randomness[j] % NUM_LETTERS];
        
        std::memset(tmp_path, 0, MAX_PATH);

        DWORD rp = GetTempPathA(MAX_PATH, tmp_path);
        if (!(rp > MAX_PATH || rp == 0)) {
            snprintf(this->path, MAX_PATH, "%s/%s", tmp_path, template_XXXXXX_str.c_str());

            this->fd = CreateFile (
                this->path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_TEMPORARY,
                NULL
            );

            if (this->fd == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    /* Any other error will apply also to other names we might
                    try, and there are 2^32 or so of them, so give up now. */
                    delete this->path;
                    this->path = nullptr;
                    this->fd = this->invalid_fd;
                    return;
                }
                continue;
            }
            return;
        }
        // we failed to get a temporary path
        errno = save_errno;
        delete[] this->path;
        this->fd = this->invalid_fd;
        return;
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    errno = EEXIST;
    delete[] this->path;
    this->fd = this->invalid_fd;
    return;
#else
    const char * tmp_dir;
    /*
        ISO/IEC 9945 (POSIX): The path supplied by the first environment variable found in the list
         TMPDIR, TMP, TEMP, TEMPDIR.
        
        If none of these are found, "/tmp", or, if macro __ANDROID__ is defined, "/data/local/tmp"
    */
    tmp_dir = getenv("TMPDIR");
    if (tmp_dir == nullptr) {
        tmp_dir = getenv("TMP");
        if (tmp_dir == nullptr) {
            tmp_dir = getenv("TEMP");
            if (tmp_dir == nullptr) {
                tmp_dir = getenv("TEMPDIR");
                if (tmp_dir == nullptr) {
#ifdef __ANDROID__
                    tmp_dir = "/data/local/tmp";
#else
                    tmp_dir = "/tmp";
#endif
                }
            }
        }
    }
    auto t = std::string(tmp_dir) + "/" + template_XXXXXX_str;
    this->path = strdup(t.c_str());
    this->fd = mkstemp(this->path);
    if (this->fd < 0) {
        free(this->path);
        this->path = nullptr;
        this->fd = this->invalid_fd;
    }
#endif
}

const char * TempFile::get_path() const {
    return const_cast<const char *>(this->path);
}
#ifdef _WIN32
HANDLE
#else
int
#endif
TempFile::get_handle() const {
    return this->fd;
}

TempFile::~TempFile() {
    if (this->fd != this->invalid_fd) {
#ifdef _WIN32
        CloseHandle(this->fd);
#else
        close(this->fd);
#endif
        this->fd = this->invalid_fd;
    }
    if (this->path != nullptr) {
#ifdef _WIN32
        DeleteFile(this->path);
        delete[] this->path;
#else
        free(this->path);
#endif
        this->path = nullptr;
    }
}