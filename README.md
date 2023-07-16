# TempFile
a C++ Cross-Platform temporary file library

# example

```cpp
#include <tmpfile.h>

#include <iostream>
#include <vector>

struct TmpFileHolder {

    // multiple `TempFile` objects can exist

    std::vector<std::pair<std::string, TempFile>> files;

    void file(const std::string & id, const std::string & name) {
        files.push_back(std::pair<std::string, TempFile>(id, TempFile(name)));
    }

    void dir(const std::string & id, const std::string & dir, const std::string & name) {
        files.push_back(std::pair<std::string, TempFile>(id, TempFile(dir, name)));
    }

    ~TmpFileHolder() {
        for(auto pair : files) {
            print(pair.first, pair.second);
        }
    }

    private:

    void print(const std::string & id, TempFile file) {
        std::cout << "temporary file id: " << id << std::endl;
        auto path = file.get_path();
        std::cout << "    is valid: " << (file.is_valid() ? "true" : "false");
        auto handle = file.get_handle();
        #ifdef _WIN32
        std::cout << ", handle: " << (handle == INVALID_HANDLE_VALUE ? "INVALID_HANDLE_VALUE" : handle);
        #else
        std::cout << ", handle: " << std::to_string(handle);
        #endif
        std::cout << ", path: " << (path.length() == 0 ? "nullptr" : path) << std::endl;
        std::cout << std::endl;
    }
};

int main() {

    TmpFileHolder files;

    files.file("this file should exist", "--");
    files.file("this file should exist, no name given", "");
    files.file("this file should exist #1", "--");
    files.dir("this file in the directory /tmp/foobar should exist", "/tmp/foobar", "--");
    files.dir("this file in the directory /tmp/foobar should exist, no name given", "/tmp/foobar", "");
    files.dir("this file in the directory /tmp/foobar should exist #2", "/tmp/foobar", "--");
    files.dir("this file in the directory /tmp/foo should NOT exist", "/tmp/foo", "--");
    files.dir("this file in the directory /tmp/fejbf should NOT exist", "/tmp/fejbf", "--");
    files.dir("this file in the directory /tmpegwaeg/r32htg73q489 should NOT exist #3", "/tmpegwaeg/r32htg73q489", "--");

    return 0;
}
```

## possible output

```
$ clang++ example.cpp -I include -rpath $(pwd)/release_BUILD ./release_BUILD/libtmpfile.so -o example && echo && ./example

temporary file id: this file should exist
    is valid: true, handle: 3, path: /tmp/--8DabmK

temporary file id: this file should exist, no name given
    is valid: true, handle: 4, path: /tmp/hSjESJ

temporary file id: this file should exist #1
    is valid: true, handle: 5, path: /tmp/--Llm62H

temporary file id: this file in the directory /tmp/foobar should exist
    is valid: true, handle: 6, path: /tmp/foobar/--LFxzTG

temporary file id: this file in the directory /tmp/foobar should exist, no name given
    is valid: true, handle: 7, path: /tmp/foobar/ZFNYWG

temporary file id: this file in the directory /tmp/foobar should exist #2
    is valid: true, handle: 8, path: /tmp/foobar/--E85qPH

temporary file id: this file in the directory /tmp/foo should NOT exist
    is valid: false, handle: -1, path: /tmp/foo/--LU5jpH

temporary file id: this file in the directory /tmp/fejbf should NOT exist
    is valid: false, handle: -1, path: /tmp/fejbf/--9SiwJJ

temporary file id: this file in the directory /tmpegwaeg/r32htg73q489 should NOT exist #3
    is valid: false, handle: -1, path: /tmpegwaeg/r32htg73q489/--H0y6eH
```

# public api

```cpp
class TempFile {
    // ...

    public:

    TempFile();
    TempFile(const std::string & template_prefix);
    TempFile(const std::string & dir, const std::string & template_prefix);
    ~TempFile();

    bool is_valid();

    bool construct(const std::string & template_prefix);
    bool construct(const std::string & dir, const std::string & template_prefix);

    const std::string & get_path() const;

    #ifdef _WIN32
    HANDLE get_handle() const;
    #else
    int get_handle() const;
    #endif

};
```

the platform specific handle (`file descriptor on unix, HANDLE on windows`) can be obtained with `get_handle`, unspecified if creation fails

the absolute path to the temporary file can be obtained via `get_path`
- if creation fails this will contain the absolute path to the temporary file that was attempted to be created

the handle and path are automatically cleaned up (`closed and deleted from filesystem`) when the `TempFile` object goes out of scope

# construction

`const std::string & dir` argument
- if  `dir` is `nullptr` or `""` then an `implementation specific directory` is chosen
- the `calling process` must be able to `read` and `write` to the specified `dir`
- `dir` must `exist` at the time of the call, `TempFile will not create it for you`

`const std::string & template_prefix` argument
- if  `template_prefix` is `nullptr` or `""` then the temporary file will be created with an `unspecified prefix`

the constructor `TempFile()` does not create any temporary file, use `construct` to create one
- `TempFile tmp; ... ; tmp.construct("my_file");`
- `TempFile tmp; ... ; tmp.construct("./dir", "my_file");`
- `./dir` must exist, TempFile will not create it for you

the constructor `TempFile(const std::string & template_prefix)` will create a temporary file with the `specified prefix`
- `TempFile tmp("my_file");`
- `TempFile tmp("");` and `TempFile tmp(nullptr);` will both create a temporary file with an `unspecified prefix`
- `tmp.construct("");` and `tmp.construct(nullptr);` will both create a temporary file with an `unspecified prefix`
- in all cases of the `(const std::string & template_prefix)` call, an `implementation specific directory` is chosen to create the temporary file in

the constructor `TempFile(const std::string & dir, const std::string & template_prefix)` will create a temporary file with the `specified prefix` in the `specified directory`
- `TempFile tmp("./dir", "my_file");`
- `tmp.construct("./dir", "my_file");`


# constuct

as noted above, `construct` can be used to manually create a temporary file and is called by both constructors

`is_valid` can be used to detect if the temporary file is actually created or needs to be created

if `is_valid()` returns `true` then `construct` will `no-op` and return `true`

otherwise `construct` will `continue to attempt to create the temporary file until it encounters an error that it considers to be fatal`

if such an error is returned, `false` is returned, the the user should check `get_handle` on unix, `errno` on unix and windows, and `GetLastError` on windows

if `construct` is called a second time, then it will `clean up and try again`, if a `fatal error` has `previously occured` it will be ignored

this means if `construct` detects a `fatal error` then calling it again with the same arguments will usually produce the same `fatal error`, depending on what the `fatal error` itself is

as a result, the user should `attempt to reset or resolve any errors` before calling `construct` again (for windows, `SetLastError`)

if no error is encountered and the temporary file is successfully created, then `construct` returns `true`

# internals

under the hood we use
- Linux - `mkstemp` + tmp dir lookup
- Windows - a `for` loop + `GetTempPathA` + `CreateFile` + an algorithm to generate the `XXXXXX` replacement characters

on posix systems (linux) we look up the tmp dir using the following approach
- search the environmental variables for `TMPDIR`, `TMP`, `TEMP`, and `TEMPDIR`, and use the first one found
- if none of these are found, if the macro `__ANDROID__` is defined, use `/data/local/tmp`, otherwise use `/tmp`
