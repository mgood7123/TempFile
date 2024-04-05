#ifndef LIB_TMPFILE_H
#define LIB_TMPFILE_H

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <memory>
#include <string>

class TempFile;
class TempFileFD;
class TempFileFILE;

#define TEMP_FILE_OPEN_MODE_READ (1 << 0)
#define TEMP_FILE_OPEN_MODE_WRITE (1 << 1)
#define TEMP_FILE_OPEN_MODE_BINARY (1 << 2)

class TempFile {
private:
    struct CleanUp {

        std::string path;

        bool fatal_path = false;

        bool detached = false;

        bool log_create_close = false;

#if defined(_WIN32)
        HANDLE fd;
#else
        int fd;
#endif

        CleanUp();

        bool is_valid() const;

        void detach();

        void reset_fd();

        void reset_path();

        void reset();

        ~CleanUp();
    };

    std::shared_ptr<CleanUp> data;


public:

    static std::string TempDir();

    TempFile();
    TempFile(const std::string & dir, const std::string & template_prefix);
    TempFile(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    TempFile(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    TempFile(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);

    bool is_valid() const;

    bool construct(const std::string & dir, const std::string & template_prefix);
    bool construct(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);

    // pointers are implicitly convertible to bool
    inline TempFile(const std::string & dir, const std::string & template_prefix, char * template_suffix) : TempFile(dir, template_prefix, std::string(template_suffix)) {}
    inline TempFile(const std::string & dir, const std::string & template_prefix, const char * template_suffix) : TempFile(dir, template_prefix, std::string(template_suffix)) {}
    inline bool construct(const std::string & dir, const std::string & template_prefix, char * template_suffix) { return construct(dir, template_prefix, std::string(template_suffix)); }
    inline bool construct(const std::string & dir, const std::string & template_prefix, const char * template_suffix) { return construct(dir, template_prefix, std::string(template_suffix)); }

    const std::string & get_path() const;

    TempFile & detach();

    #if defined(_WIN32)
    HANDLE get_handle() const;
    #else
    int get_handle() const;
    #endif

    TempFile & reset();

    TempFileFD toFD();
    TempFileFILE toFILE();
    TempFileFILE toFILE(int open_mode);
    friend TempFileFD;
    friend TempFileFILE;
};

class TempFileFD {
private:
    struct CleanUp {

        std::string path;

        bool fatal_path = false;

        bool detached = false;

        bool log_create_close = false;

        int fd;

        CleanUp();

        bool is_valid() const;

        void detach();

        void reset_fd();

        void reset_path();

        void reset();

        ~CleanUp();
    };

    std::shared_ptr<CleanUp> data;

public:

    static inline std::string TempDir() { return TempFile::TempDir(); }

    TempFileFD();
    TempFileFD(const std::string & dir, const std::string & template_prefix);
    TempFileFD(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    TempFileFD(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    TempFileFD(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);

    bool is_valid() const;

    bool construct(const std::string & dir, const std::string & template_prefix);
    bool construct(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);

    // pointers are implicitly convertible to bool
    inline TempFileFD(const std::string & dir, const std::string & template_prefix, char * template_suffix) : TempFileFD(dir, template_prefix, std::string(template_suffix)) {}
    inline TempFileFD(const std::string & dir, const std::string & template_prefix, const char * template_suffix) : TempFileFD(dir, template_prefix, std::string(template_suffix)) {}
    inline bool construct(const std::string & dir, const std::string & template_prefix, char * template_suffix) { return construct(dir, template_prefix, std::string(template_suffix)); }
    inline bool construct(const std::string & dir, const std::string & template_prefix, const char * template_suffix) { return construct(dir, template_prefix, std::string(template_suffix)); }

    const std::string & get_path() const;

    TempFileFD & detach();

    int get_handle() const;
    
    TempFileFD & reset();

    TempFile toHandle();
    TempFileFILE toFILE();
    TempFileFILE toFILE(int open_mode);
    friend TempFile;
    friend TempFileFILE;
};

class TempFileFILE {
private:
    struct CleanUp {

        std::string path;

        bool fatal_path = false;

        bool detached = false;

        bool log_create_close = false;

        FILE* fd;

        CleanUp();

        bool is_valid() const;

        void detach();

        void reset_fd();

        void reset_path();

        void reset();

        ~CleanUp();
    };

    std::shared_ptr<CleanUp> data;

public:

    static inline std::string TempDir() { return TempFile::TempDir(); }

    bool is_valid() const;

    // generated by gen.exe -- header start

    TempFileFILE();
    TempFileFILE(const std::string & dir);
    TempFileFILE(const std::string & dir, const std::string & template_prefix);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, int open_mode);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, int open_mode, bool log_create_close);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, char * template_suffix);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, const char * template_suffix);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);
    TempFileFILE(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode, bool log_create_close);

    bool construct();
    bool construct(const std::string & dir);
    bool construct(const std::string & dir, const std::string & template_prefix);
    bool construct(const std::string & dir, const std::string & template_prefix, int open_mode);
    bool construct(const std::string & dir, const std::string & template_prefix, bool log_create_close);
    bool construct(const std::string & dir, const std::string & template_prefix, int open_mode, bool log_create_close);
    bool construct(const std::string & dir, const std::string & template_prefix, char * template_suffix);
    bool construct(const std::string & dir, const std::string & template_prefix, const char * template_suffix);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close);
    bool construct(const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode, bool log_create_close);

    // generated by gen.exe -- header end

    const std::string & get_path() const;

    TempFileFILE & detach();

    FILE* get_handle() const;
    
    TempFileFILE & reset();

    TempFile toHandle();
    TempFileFD toFD();
    friend TempFile;
    friend TempFileFD;
};
#endif // LIB_TMPFILE_H