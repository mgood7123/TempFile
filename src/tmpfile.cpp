#include <limits.h> // CHAR_BIT

#include <iostream>

// sanity check
#if CHAR_BIT != 8
#error system does not support 8 bit addressing
#endif

#include <tmpfile.h>

#ifdef _WIN32
/* Load size_t on windows */
#include <crtdefs.h>
#else
#include <unistd.h>
#endif /* _WIN32 */

#include <string>

/*
 * Write `n` bytes of high quality random bytes to `buf`
 */
extern "C" int randombytes(void *buf, size_t n);

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

static void brain_damaged_fillrand(unsigned std::string &buf, unsigned int len)
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

#else
#include <string.h> // strdup
#include <stdlib.h> // free
#endif

struct SaveError {
    bool reset_errno = true;
    int errno_;
#ifdef _WIN32
    bool reset_last_error = true;
    int last_error;
#endif

    SaveError()
    {
        set_errno();
#ifdef _WIN32
        set_last_error();
#endif
    }

    void set_errno() {
        errno_ = errno;
    }

    void set_errno(int errno_) {
        this->errno_ = errno_;
    }

    void no_errno() {
        reset_errno = false;
    }

    int get_errno() {
        return errno_;
    }

    #ifdef _WIN32
    void set_last_error() {
        // preserve errno in case the user calls set_errno after
        int e = errno;
        last_error = GetLastError();
        errno = e;
    }

    void set_last_error(int last_error) {
        this->last_error = last_error;
    }

    void no_last_error() {
        reset_last_error = false;
    }

    int get_last_error() {
        return last_error;
    }

    #endif

    void disable() {
        no_errno();
        #ifdef _win32
        no_last_error();
        #endif
    }

    ~SaveError() {
        #ifdef _WIN32
        if (reset_last_error)
            SetLastError(last_error);
        #endif
        if (reset_errno)
            errno = errno_;
    }
};

TempFile::CleanUp::CleanUp() {
#ifdef _WIN32
    fd = INVALID_HANDLE_VALUE;
#else
    fd = -1;
#endif
}

bool TempFile::CleanUp::is_valid() const {
    return
#ifdef _WIN32
    fd != INVALID_HANDLE_VALUE
#else
    fd >= 0
#endif
    && path.length() != 0;
}

void TempFile::CleanUp::detach() {
    detached = true;
}

void TempFile::CleanUp::reset_fd() {
#ifdef _WIN32
    if (fd != INVALID_HANDLE_VALUE) {
        SaveError e;
        CloseHandle(fd);
    }
    fd = INVALID_HANDLE_VALUE;
#else
    if (fd >= 0) {
        SaveError e;
        close(fd);
    }
    fd = -1;
#endif
}

void TempFile::CleanUp::reset_path() {
    if (!fatal_path && path.length() != 0) {
        SaveError e;
        if (log_create_close) {
            if (detached) {
                std::cout << "detaching temporary file: " << path << std::endl;
            } else {
                std::cout << "deleting temporary file: " << path << std::endl;
            }
        }
#ifdef _WIN32
        if (!detached) DeleteFile(path.c_str());
#else
        if (!detached) unlink(path.c_str());
#endif
    }
    path = {};
    detached = false;
}

void TempFile::CleanUp::reset() {
    reset_fd();
    reset_path();
}

TempFile::CleanUp::~CleanUp() {
    reset();
}

TempFile::TempFile() {
    data = std::make_shared<CleanUp>();
}

TempFile::TempFile(const std::string & template_prefix) {
    data = std::make_shared<CleanUp>();
    construct(template_prefix);
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix);
}

TempFile::TempFile(const std::string & template_prefix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(template_prefix, log_create_close);
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, log_create_close);
}

bool TempFile::is_valid() const {
    return this->data->is_valid();
}

bool TempFile::construct(const std::string & template_prefix) {
    return construct(template_prefix, false);
}

bool TempFile::construct(const std::string & template_prefix, bool log_create_close) {
    if (this->data->is_valid()) {
        // return true if we are already set-up
        return true;
    }

#ifdef _WIN32
    char tmp_dir[MAX_PATH];
    std::memset(tmp_dir, 0, MAX_PATH);
    DWORD rp = GetTempPathA(MAX_PATH, tmp_dir);
    if (rp > MAX_PATH || rp == 0) {
        // failed to get temporary path
        // it is reasonable to assume subsequent attempts to obtain the path will fail until fixed by user
        return false;
    }
#else
    /*
        ISO/IEC 9945 (POSIX): The path supplied by the first environment variable found in the list
        TMPDIR, TMP, TEMP, TEMPDIR.
        
        If none of these are found, "/tmp", or, if macro __ANDROID__ is defined, "/data/local/tmp"
    */
    const char * tmp_dir = getenv("TMPDIR");
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
#endif
    return construct(tmp_dir, template_prefix, log_create_close);
}

#include <random>
#include <functional> // std::hash
#include <chrono> // epoch
#include <iostream>
#include <iterator>

static char * rng_get_fallback_buf() {
    static char buf[1];
    return buf;
}

static int rng_get_fallback() {
    static auto unused = randombytes(rng_get_fallback_buf(), sizeof(char));
    return static_cast<int>(rng_get_fallback_buf()[0]);
}

static auto rng_get() {
    static int r = std::random_device()();
    if (r == 0) {
        static int r2 = rng_get_fallback();
        return r2 == 0 ? 1 : r2;
    } else {
        return r;
    }
}

static void * get_stack_var() {
    int foo = 6;
    return std::addressof(foo);
}

// static initialization order is unspecified, use macros to garentee order
using SEED_TYPE = long unsigned int;

template <typename H>
std::size_t h(const H & ha) {
    std::size_t hash = std::hash<H>()(ha);
    return hash == 0 ? 1 : hash;
}

#define HASH_CSTR(c) h(std::string(reinterpret_cast<const char*>(c)))
#define HASH_CSTRL(c, l) h(std::string(reinterpret_cast<const char*>(c), l))
#define HASH_INT(c) h(static_cast<int>(c))
#define HASH_PTR(c) h(reinterpret_cast<size_t>(c))

#define seed_2 (std::chrono::system_clock::now().time_since_epoch().count() == 0 ? 1 : std::chrono::system_clock::now().time_since_epoch().count())
#define seed_1 rng_get() * rng_get() * seed_2
#define seed_3 HASH_CSTR(__FILE__) + HASH_CSTR(__DATE__) + seed_1
#define seed_4 HASH_INT(__LINE__) + HASH_CSTR(__DATE__) + seed_1
#define seed_5 HASH_CSTR(__FILE__) + HASH_INT(__LINE__) + HASH_CSTR(__DATE__) + seed_1
#define seed_6 HASH_PTR(&rng_get) + seed_1
#define seed_7 reinterpret_cast<size_t>(get_stack_var()) + seed_1
#define seed_8 seed_2 * seed_1
#define seed_9 seed_2 * seed_2
#define seed \
    static_cast<SEED_TYPE>(seed_1), static_cast<SEED_TYPE>(seed_2), static_cast<SEED_TYPE>(seed_3), \
    static_cast<SEED_TYPE>(seed_4), static_cast<SEED_TYPE>(seed_5), static_cast<SEED_TYPE>(seed_6), \
    static_cast<SEED_TYPE>(seed_7), static_cast<SEED_TYPE>(seed_8), static_cast<SEED_TYPE>(seed_9)

// use given seq to generate random seeds to produce more random seeds

class SeedList
{
    public:
    std::vector<std::seed_seq*> seeds;
    SeedList() {}

    // mutate the seed N times, the original seed must have randomness
    std::seed_seq & get(int mutation_count, SEED_TYPE s1, SEED_TYPE s2, SEED_TYPE s3, SEED_TYPE s4, SEED_TYPE s5, SEED_TYPE s6, SEED_TYPE s7, SEED_TYPE s8, SEED_TYPE s9) {
        // std::cout << "creating seed" << std::endl;
        std::vector<SEED_TYPE> current_seed_vec = {s1, s2, s3, s4, s5, s6, s7, s8, s9};
        // std::cout << "seed: { ";
        // std::copy(current_seed_vec.begin(), current_seed_vec.end(), std::ostream_iterator<SEED_TYPE>(std::cout, ", "));
        // std::cout << " }" << std::endl;


        for (int mutation_c = 0; mutation_c < mutation_count; mutation_c ++) {

            auto tmp_seq = std::seed_seq(current_seed_vec.begin(), current_seed_vec.end());

            std::mt19937_64 s {tmp_seq};
            
            current_seed_vec = {s(), s(), s(), s(), s(), s(), s(), s(), s()};
            
            // std::cout << "mutated seed: { ";
            // std::copy(current_seed_vec.begin(), current_seed_vec.end(), std::ostream_iterator<SEED_TYPE>(std::cout, ", "));
            // std::cout << " }" << std::endl;
        }

        seeds.push_back(new std::seed_seq(current_seed_vec.begin(), current_seed_vec.end()));
        return *seeds.back();
    }

    std::seed_seq & get(SEED_TYPE s1, SEED_TYPE s2, SEED_TYPE s3, SEED_TYPE s4, SEED_TYPE s5, SEED_TYPE s6, SEED_TYPE s7, SEED_TYPE s8, SEED_TYPE s9) {
        return get(1, s1, s2, s3, s4, s5, s6, s7, s8, s9);
    }

    ~SeedList() {
        for(auto s : seeds) {
            // std::cout << "deleting seed" << std::endl;
            delete s;
        }
    }
};


// 1 mutation is sufficent to generate a randomized seed
//   for all seed components as long as one component contains true randomness
//  however we can mutate more times if needed


std::mt19937_64& rng() {
    static SeedList seq_list;
    static std::mt19937_64 rng_ {seq_list.get(1, seed)};
    return rng_;
}

#define LETTER_DIST letters[std::uniform_int_distribution<> dist {0, NUM_LETTERS-1} (rng())]

bool TempFile::construct(const std::string & dir, const std::string & template_prefix) {
    return construct(dir, template_prefix, false);
}

bool TempFile::construct(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    if (dir.length() == 0) {
        return construct(template_prefix, log_create_close);
    }

    if (this->data->is_valid()) {
        // return true if we are already set-up
        return true;
    }

    SaveError error;

    // we dont care if we get any errors here, if we fail to clean up then we should not consider this an error
    this->data->reset();

    this->data->log_create_close = log_create_close;

    // we have cleaned up

    std::string path = {};
    path += dir;
    path += "/";
    path += template_prefix;
    path += "XXXXXX";
    char * XXXXXX = &path[path.length()-6];

#ifdef _WIN32

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

    unsigned int i;
#endif

    while (true) {

        // if we are invalid we need to clean up and try again
        this->data->reset();

#ifdef _WIN32

#ifndef TMP_MAX
#define TMP_MAX 238328
#endif

        for (i = 0; i < TMP_MAX; ++i) {
            
            /* Get some random data.  */
            XXXXXX[0] = LETTER_DIST; // 1
            XXXXXX[1] = LETTER_DIST; // 2
            XXXXXX[2] = LETTER_DIST; // 3
            XXXXXX[3] = LETTER_DIST; // 4
            XXXXXX[4] = LETTER_DIST; // 5
            XXXXXX[5] = LETTER_DIST; // 6
            
            this->data->fd = CreateFile (
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_TEMPORARY,
                NULL
            );

            if (this->data->fd == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    /* Any other error will apply also to other names we might
                    try, and there are 2^32 or so of them, so give up now. */

                    // fd is already invalid thus no need to reset it

                    error = {}; // save current error, and restore after move

                    this->data->path = std::move(path); // so the user can see what path may have caused the error

                    // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                    this->data->fatal_path = true;

                    return false;
                }
                // file exists, try again
                continue;
            }
            // we got a valid handle, and we have a valid path
            this->data->path = std::move(path);
            if (this->data->log_create_close) {
                std::cout << "created temporary file: " << this->data->path << std::endl;
            }
            return true;
        }

        /* We got out of the loop because we ran out of combinations to try.  */

        // fd is already invalid thus no need to reset it
        error.set_last_error();
        error.set_errno(EEXIST);
        this->data->path = std::move(path); // so the user can see what path may have caused the error

        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        this->data->fatal_path = true;

        return false;
#else
        this->data->fd = mkstemp(&path[0]);

        if (this->data->fd < 0) {
            if (this->data->fd == -1) {
                error = {}; // save current error, and restore after move
                this->data->path = std::move(path); // so the user can see what path may have caused the error

                // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                this->data->fatal_path = true;

                return false;
            }
            goto LOOP_CONTINUE;
        }
        this->data->path = std::move(path);
        if (this->data->log_create_close) {
            std::cout << "created temporary file: " << this->data->path << std::endl;
        }
        return true;
#endif
        LOOP_CONTINUE:
        continue;
    }
    // we should not get to here
    error = {}; // save current error, and restore after move

    this->data->path = std::move(path); // so the user can see what path may have caused the error

    // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
    this->data->fatal_path = true;

    return false;
}

const std::string & TempFile::get_path() const {
    return this->data->path;
}

void TempFile::detach() {
    this->data->detach();
}

#ifdef _WIN32
HANDLE
#else
int
#endif
TempFile::get_handle() const {
    return this->data->fd;
}
