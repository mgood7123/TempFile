#ifndef LIB_TEMPORARY_FILE_H
#define LIB_TEMPORARY_FILE_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <memory>
#include <string>

class TempFile {
private:
    struct CleanUp {

        std::string path;

        bool fatal_path = false;

        bool detached = false;

#ifdef _WIN32
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

    TempFile();
    TempFile(const std::string & template_prefix);
    TempFile(const std::string & dir, const std::string & template_prefix);

    bool is_valid() const;

    bool construct(const std::string & template_prefix);
    bool construct(const std::string & dir, const std::string & template_prefix);

    const std::string & get_path() const;

    void detach();

    #ifdef _WIN32
    HANDLE get_handle() const;
    #else
    int get_handle() const;
    #endif
    
};

#endif