# TempFile
a C++ Cross-Platform temporary file library

# public api

```cpp
class TempFile {
    // ...

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
```


the platform specific handle can be obtained with `get_handle`

the absolute path to the temporary file can be obtained via `get_path`

the handle and path are automatically cleaned up when the `TempFile` object goes out of scope

# construction

simply construct via `TempFile tmp("my_file");` and the `TempFile` will create a temporary file for you

construct via `TempFile tmp;` and use `tmp.construct("my_file");` to create the temporary object when needed

both `TempFile tmp("my_file"); TempFile tmp2("my_file");` and `tmp.construct("my_file"); tmp2.construct("my_file");` are valid and will construct unique temporary files with different suffixes, eg `my_file543BN2` and `my_file4u3gim`

`is_handle_valid` can be used to detect if the temporary file is actually created or needs to be created

`construct` can be used to manually create a temporary file

if `is_valid_handle()` returns `true` then `construct` will `no-op` and return `true`

otherwise `construct` will `continue to attempt to create the temporary file until it encounters an error that it considers to be fatal`

if such an error is returned, `false` is returned, the the user should check `get_handle` on unix, `errno` on unix and windows, and `GetLastError` on windows

if `construct` is called a second time, then it will `clean up and try again`, if a `fatal error` has `previously occured` it will be ignored

this means if `construct` detects a `fatal error` then calling it again with the same arguments will usually produce the same `fatal error`, depending on what the `fatal error` itself is

if no error is encountered and the temporary file is successfully created, then `construct` returns `true`


the `const char * template_prefix` constructor calls `construct` in a loop until it succeeds

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