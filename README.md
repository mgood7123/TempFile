# TempFile
a C++ Cross-Platform temporary file library

```cpp
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
    const char * get_path() const;
    #ifdef _WIN32
    HANDLE get_handle() const;
    #else
    int get_handle() const;
    #endif
    ~TempFile();
};
```

simply construct via `TempFile tmp("my_file");` and the `TempFile` will create a temporary file for you

the platform specific handle can be obtained with `get_handle`

the absolute path to the temporary file can be obtained via `get_path`

the handle and path are automatically cleaned up when the `TempFile` object goes out of scope

# internals

under the hood we use
- Linux - `mkstemp` + tmp dir lookup
- Windows - a `for` loop + `GetTempPathA` + `CreateFile` + an algorithm to generate the `XXXXXX` replacement characters

on posix systems (linux) we look up the tmp dir using the following approach

```cpp
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
```