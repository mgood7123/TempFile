#include <tmpfile/tmpfile.h>

#include <limits.h> // CHAR_BIT

#include <iostream>

#if defined(_WIN32)
#include <crtdefs.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif /* defined(_WIN32) */

#include <string>

#if defined(_WIN32)
/* These are the characters used in temporary filenames.  */
static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
#define NUM_LETTERS (62)

static inline int LETTER_ID() {
    static std::uniform_int_distribution<int> dist{ 0, NUM_LETTERS - 1 };
    return dist(rng());
}

#define LETTER_DIST letters[LETTER_ID()]
#else
#include <string.h> // strdup
#include <stdlib.h> // free
#endif

struct SaveError {
    bool reset_errno = true;
    int errno_;
#if defined(_WIN32)
    bool reset_last_error = true;
    int last_error;
#endif

    SaveError()
    {
        set_errno();
#if defined(_WIN32)
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

    #if defined(_WIN32)
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
        #if defined(_WIN32)
        if (reset_last_error)
            SetLastError(last_error);
        #endif
        if (reset_errno)
            errno = errno_;
    }
};

std::string TempFile::TempDir() {
#if defined(_WIN32)
    char * tmp_dir = (char*)calloc(1, MAX_PATH+1);
    if (tmp_dir == nullptr) {
        throw std::bad_alloc();
    }
    DWORD rp = GetTempPathA(MAX_PATH, tmp_dir);
    if (rp > MAX_PATH || rp == 0) {
        free(tmp_dir);
        // failed to get temporary path
        // it is reasonable to assume subsequent attempts to obtain the path will fail until fixed by user
        return "";
    }
    std::string t = tmp_dir;
    free(tmp_dir);
    return t;
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
    return tmp_dir;
#endif
}

TempFile::CleanUp::CleanUp() {
#if defined(_WIN32)
    fd = INVALID_HANDLE_VALUE;
#else
    fd = -1;
#endif
}

bool TempFile::CleanUp::is_valid() const {
    return
#if defined(_WIN32)
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
#if defined(_WIN32)
    if (fd != INVALID_HANDLE_VALUE) {
        SaveError e;
        if (!detached) CloseHandle(fd);
    }
    fd = INVALID_HANDLE_VALUE;
#else
    if (fd >= 0) {
        SaveError e;
        if (!detached) close(fd);
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
#if defined(_WIN32)
        if (!detached) DeleteFile(path.c_str());
#else
        if (!detached) unlink(path.c_str());
#endif
    }
    path = {};
}

void TempFile::CleanUp::reset() {
    reset_fd();
    reset_path();
    detached = false;
}

TempFile::CleanUp::~CleanUp() {
    reset();
}

#include <random>
#include <functional> // std::hash
#include <chrono> // epoch
#include <iostream>
#include <iterator>

static char* rng_get_fallback_buf() {
    static char buf[1];
    return buf;
}

/*
 * Write `n` bytes of high quality random bytes to `buf`
 */
extern "C" int randombytes(void* buf, size_t n);

static int rng_get_fallback() {
    static auto unused = randombytes(rng_get_fallback_buf(), sizeof(char));
    return static_cast<int>(rng_get_fallback_buf()[0]);
}

static auto rng_get() {
    static int r = std::random_device()();
    if (r == 0) {
        static int r2 = rng_get_fallback();
        return r2 == 0 ? 1 : r2;
    }
    else {
        return r;
    }
}

static void * get_stack_var() {
    int foo = 6;
    return std::addressof(foo);
}

// static initialization order is unspecified, use macros to guarantee order
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


// 1 mutation is sufficient to generate a randomized seed
//   for all seed components as long as one component contains true randomness
//  however we can mutate more times if needed

static inline std::mt19937_64& rng() {
    static SeedList seq_list;
    static std::mt19937_64 rng_ {seq_list.get(1, seed)};
    return rng_;
}

TempFile::TempFile() {
    data = std::make_shared<CleanUp>();
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix);
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, log_create_close);
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix);
}

TempFile::TempFile(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix, log_create_close);
}

bool TempFile::is_valid() const {
    return this->data->is_valid();
}

bool TempFile::construct(const std::string & dir, const std::string & template_prefix) {
    return construct(dir, template_prefix, "", false);
}

bool TempFile::construct(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    return construct(dir, template_prefix, "", log_create_close);
}

bool TempFile::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    return construct(dir, template_prefix, template_suffix, false);
}

bool TempFile::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    if (dir.length() == 0) {
        return construct(TempDir(), template_prefix, template_suffix, log_create_close);
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
    path += template_suffix;

#if defined(_WIN32)
    char * XXXXXX = &path[path.length()-template_suffix.length()-6];

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

#if defined(_WIN32)

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
                FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
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
        this->data->fd = mkstemps(&path[0], template_suffix.length());

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

TempFile & TempFile::detach() {
    this->data->detach();
    return *this;
}

#if defined(_WIN32)
HANDLE
#else
int
#endif
TempFile::get_handle() const {
    return this->data->fd;
}

TempFile & TempFile::reset() {
    this->data->reset();
    return *this;
}

// FD

TempFileFD::CleanUp::CleanUp() {
    fd = -1;
}

bool TempFileFD::CleanUp::is_valid() const {
    return fd >= 0 && path.length() != 0;
}

void TempFileFD::CleanUp::detach() {
    detached = true;
}

void TempFileFD::CleanUp::reset_fd() {
    if (fd >= 0) {
        SaveError e;
#if defined(_WIN32)
        if (!detached) _close(fd);
#else
        if (!detached) close(fd);
#endif
    }
    fd = -1;
}

void TempFileFD::CleanUp::reset_path() {
    if (!fatal_path && path.length() != 0) {
        SaveError e;
        if (log_create_close) {
            if (detached) {
                std::cout << "detaching temporary file: " << path << std::endl;
            } else {
                std::cout << "deleting temporary file: " << path << std::endl;
            }
        }
#if defined(_WIN32)
        if (!detached) DeleteFile(path.c_str());
#else
        if (!detached) unlink(path.c_str());
#endif
    }
    path = {};
}

void TempFileFD::CleanUp::reset() {
    reset_fd();
    reset_path();
    detached = false;
}

TempFileFD::CleanUp::~CleanUp() {
    reset();
}

TempFileFD::TempFileFD() {
    data = std::make_shared<CleanUp>();
}

TempFileFD::TempFileFD(const std::string & dir, const std::string & template_prefix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix);
}

TempFileFD::TempFileFD(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, log_create_close);
}

TempFileFD::TempFileFD(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix);
}

TempFileFD::TempFileFD(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix, log_create_close);
}

bool TempFileFD::is_valid() const {
    return this->data->is_valid();
}

bool TempFileFD::construct(const std::string & dir, const std::string & template_prefix) {
    return construct(dir, template_prefix, "", false);
}

bool TempFileFD::construct(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    return construct(dir, template_prefix, "", log_create_close);
}

bool TempFileFD::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    return construct(dir, template_prefix, template_suffix, false);
}

bool TempFileFD::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    if (dir.length() == 0) {
        return construct(TempDir(), template_prefix, template_suffix, log_create_close);
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
    path += template_suffix;

#if defined(_WIN32)
    char * XXXXXX = &path[path.length()-template_suffix.length()-6];

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

#if defined(_WIN32)

#ifndef TMP_MAX
#define TMP_MAX 238328
#endif

        HANDLE handle = INVALID_HANDLE_VALUE;

        for (i = 0; i < TMP_MAX; ++i) {
            
            /* Get some random data.  */
            XXXXXX[0] = LETTER_DIST; // 1
            XXXXXX[1] = LETTER_DIST; // 2
            XXXXXX[2] = LETTER_DIST; // 3
            XXXXXX[3] = LETTER_DIST; // 4
            XXXXXX[4] = LETTER_DIST; // 5
            XXXXXX[5] = LETTER_DIST; // 6
            
            handle = CreateFile (
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
                NULL
            );

            if (handle == INVALID_HANDLE_VALUE) {
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
            this->data->fd = _open_osfhandle(handle, _O_APPEND);
            if (this->data->fd == -1) {
                error = {};

                // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                this->data->fatal_path = true;

                return false;
            }
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
        this->data->fd = mkstemps(&path[0], template_suffix.length());

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

const std::string & TempFileFD::get_path() const {
    return this->data->path;
}

TempFileFD & TempFileFD::detach() {
    this->data->detach();
    return *this;
}

int TempFileFD::get_handle() const {
    return this->data->fd;
}

TempFileFD & TempFileFD::reset() {
    this->data->reset();
    return *this;
}



// FILE*

TempFileFILE::CleanUp::CleanUp() {
    fd = nullptr;
}

bool TempFileFILE::CleanUp::is_valid() const {
    return fd != nullptr && path.length() != 0;
}

void TempFileFILE::CleanUp::detach() {
    detached = true;
}

void TempFileFILE::CleanUp::reset_fd() {
    if (fd != nullptr) {
        SaveError e;
        if (!detached) fclose(fd);
    }
    fd = nullptr;
}

void TempFileFILE::CleanUp::reset_path() {
    if (!fatal_path && path.length() != 0) {
        SaveError e;
        if (log_create_close) {
            if (detached) {
                std::cout << "detaching temporary file: " << path << std::endl;
            } else {
                std::cout << "deleting temporary file: " << path << std::endl;
            }
        }
#if defined(_WIN32)
        if (!detached) DeleteFile(path.c_str());
#else
        if (!detached) unlink(path.c_str());
#endif
    }
    path = {};
}

void TempFileFILE::CleanUp::reset() {
    reset_fd();
    reset_path();
    detached = false;
}

TempFileFILE::CleanUp::~CleanUp() {
    reset();
}

bool TempFileFILE::is_valid() const {
    return this->data->is_valid();
}

const char* OPEN_MODE_TO_FILE_MODE(int open_mode) {
    if ((open_mode & TEMP_FILE_OPEN_MODE_READ) == TEMP_FILE_OPEN_MODE_READ) return "r+";
    if ((open_mode & TEMP_FILE_OPEN_MODE_WRITE) == TEMP_FILE_OPEN_MODE_WRITE) return "w+";
    if ((open_mode & TEMP_FILE_OPEN_MODE_READ) == TEMP_FILE_OPEN_MODE_READ && (open_mode & TEMP_FILE_OPEN_MODE_WRITE) == TEMP_FILE_OPEN_MODE_WRITE) return "rw+";
    if ((open_mode & TEMP_FILE_OPEN_MODE_READ) == TEMP_FILE_OPEN_MODE_READ && (open_mode & TEMP_FILE_OPEN_MODE_BINARY) == TEMP_FILE_OPEN_MODE_BINARY) return "rb+";
    if ((open_mode & TEMP_FILE_OPEN_MODE_WRITE) == TEMP_FILE_OPEN_MODE_WRITE && (open_mode & TEMP_FILE_OPEN_MODE_BINARY) == TEMP_FILE_OPEN_MODE_BINARY) return "wb+";
    if ((open_mode & TEMP_FILE_OPEN_MODE_READ) == TEMP_FILE_OPEN_MODE_READ && (open_mode & TEMP_FILE_OPEN_MODE_WRITE) == TEMP_FILE_OPEN_MODE_WRITE && (open_mode & TEMP_FILE_OPEN_MODE_BINARY) == TEMP_FILE_OPEN_MODE_BINARY) return "rwb+";
    throw std::runtime_error("invalid mode: TEMP_FILE_OPEN_MODE_BINARY cannot be passed by itself");
}

// generated by gen.exe -- header start

TempFileFILE::TempFileFILE() {
    data = std::make_shared<CleanUp>();

}
TempFileFILE::TempFileFILE(const std::string & dir) {
    data = std::make_shared<CleanUp>();
    construct(dir);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, int open_mode) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, open_mode);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, log_create_close);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, int open_mode, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, open_mode, log_create_close);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, char * template_suffix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, std::string(template_suffix));
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, const char * template_suffix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, std::string(template_suffix));
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix, open_mode);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix, log_create_close);
}
TempFileFILE::TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode, bool log_create_close) {
    data = std::make_shared<CleanUp>();
    construct(dir, template_prefix, template_suffix, open_mode, log_create_close);
}

bool TempFileFILE::construct() {
    return construct("", "", "", TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, false);
}
bool TempFileFILE::construct(const std::string & dir) {
    return construct(dir, "", "", TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, false);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix) {
    return construct(dir, template_prefix, "", TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, false);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, int open_mode) {
    return construct(dir, template_prefix, "", open_mode, false);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, bool log_create_close) {
    return construct(dir, template_prefix, "", TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, log_create_close);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, int open_mode, bool log_create_close) {
    return construct(dir, template_prefix, "", open_mode, log_create_close);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, char * template_suffix) {
    return construct(dir, template_prefix, std::string(template_suffix));
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, const char * template_suffix) {
    return construct(dir, template_prefix, std::string(template_suffix));
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix) {
    return construct(dir, template_prefix, template_suffix, TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, false);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode) {
    return construct(dir, template_prefix, template_suffix, open_mode, false);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close) {
    return construct(dir, template_prefix, template_suffix, TEMP_FILE_OPEN_MODE_READ | TEMP_FILE_OPEN_MODE_WRITE, log_create_close);
}
bool TempFileFILE::construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode, bool log_create_close) {
    if (dir.length() == 0) {
        return construct(TempDir(), template_prefix, template_suffix, open_mode, log_create_close);
    }

// generated by gen.exe -- header end

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
    path += template_suffix;

#if defined(_WIN32)
    char * XXXXXX = &path[path.length()-template_suffix.length()-6];

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

#if defined(_WIN32)

#ifndef TMP_MAX
#define TMP_MAX 238328
#endif

        HANDLE handle = INVALID_HANDLE_VALUE;

        for (i = 0; i < TMP_MAX; ++i) {
            
            /* Get some random data.  */
            XXXXXX[0] = LETTER_DIST; // 1
            XXXXXX[1] = LETTER_DIST; // 2
            XXXXXX[2] = LETTER_DIST; // 3
            XXXXXX[3] = LETTER_DIST; // 4
            XXXXXX[4] = LETTER_DIST; // 5
            XXXXXX[5] = LETTER_DIST; // 6
            
            handle = CreateFile (
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
                NULL
            );

            if (handle == INVALID_HANDLE_VALUE) {
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
            int fd = _open_osfhandle(handle, _O_APPEND);
            if (fd == -1) {
                error = {};

                // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                this->data->fatal_path = true;

                return false;
            }
            this->data->fd = _fdopen(fd, OPEN_MODE_TO_FILE_MODE(open_mode));
            if (this->data->fd == nullptr) {
                error = {};

                // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                this->data->fatal_path = true;

                return false;
            }
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
        int fd = mkstemps(&path[0], template_suffix.length());

        if (fd < 0) {
            if (fd == -1) {
                error = {}; // save current error, and restore after move
                this->data->path = std::move(path); // so the user can see what path may have caused the error

                // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
                this->data->fatal_path = true;

                return false;
            }
            goto LOOP_CONTINUE;
        }
        this->data->path = std::move(path);
        this->data->fd = fdopen(fd, OPEN_MODE_TO_FILE_MODE(open_mode));
        if (this->data->fd == nullptr) {
            error = {};

            // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
            this->data->fatal_path = true;

            return false;
        }
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

const std::string & TempFileFILE::get_path() const {
    return this->data->path;
}

TempFileFILE & TempFileFILE::detach() {
    this->data->detach();
    return *this;
}

FILE* TempFileFILE::get_handle() const {
    return this->data->fd;
}

TempFileFILE & TempFileFILE::reset() {
    this->data->reset();
    return *this;
}

TempFileFD TempFile::toFD() {
    detach();
    TempFileFD fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    fd.data->fd = _open_osfhandle(this->data->fd, _O_APPEND);
    if (fd.data->fd == -1) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#else
    fd.data->fd = this->data->fd;
#endif
    reset();
    return fd;
}

TempFileFILE TempFile::toFILE() {
    return toFILE(TEMP_FILE_OPEN_MODE_READ);
}

TempFileFILE TempFile::toFILE(int open_mode) {
    detach();
    TempFileFILE fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    int fd_ = _open_osfhandle(this->data->fd, _O_APPEND);
    if (fd_ == -1) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
        reset();
        return fd;
    }
    fd.data->fd = _fdopen(fd_, OPEN_MODE_TO_FILE_MODE(open_mode));
    if (fd.data->fd == nullptr) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#else
    fd.data->fd = fdopen(this->data->fd, OPEN_MODE_TO_FILE_MODE(open_mode));
    if (fd.data->fd == nullptr) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#endif
    reset();
    return fd;
}

TempFile TempFileFD::toHandle() {
    detach();
    TempFile fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    fd.data->fd = _get_osfhandle(this->data->fd);
    if (fd.data->fd == INVALID_HANDLE_VALUE) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#else
    fd.data->fd = this->data->fd;
#endif
    reset();
    return fd;
}

TempFileFILE TempFileFD::toFILE() {
    return toFILE(TEMP_FILE_OPEN_MODE_READ);
}

TempFileFILE TempFileFD::toFILE(int open_mode) {
    detach();
    TempFileFILE fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    fd.data->fd = _fdopen(this->data->fd, OPEN_MODE_TO_FILE_MODE(open_mode));
#else
    fd.data->fd = fdopen(this->data->fd, OPEN_MODE_TO_FILE_MODE(open_mode));
#endif
    if (fd.data->fd == nullptr) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
    reset();
    return fd;
}

TempFileFD TempFileFILE::toFD() {
    detach();
    TempFileFD fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    fd.data->fd = _fileno(this->data->fd);
#else
    fd.data->fd = fileno(this->data->fd);
#endif
    if (fd.data->fd == -1) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
    reset();
    return fd;
}

TempFile TempFileFILE::toHandle() {
    detach();
    TempFile fd;
    if (!is_valid()) return fd;
    fd.data->path = this->data->path;
#if defined(_WIN32)
    int fd_ = _fileno(this->data->fd);
    if (fd_ == -1) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
        reset();
        return fd;
    }
    fd.data->fd = _get_osfhandle(fd_);
    if (fd.data->fd == INVALID_HANDLE_VALUE) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#else
    fd.data->fd = fileno(this->data->fd);
    if (fd.data->fd == -1) {
        // dont attempt to delete path, it did not exist at time of call and an error has prevented its creation
        fd.data->fatal_path = true;
    }
#endif
    reset();
    return fd;
}
