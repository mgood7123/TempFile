#include <tmpfile.h>

#include <iostream>
#include <vector>

struct TmpFileHolder {

    // multiple `TempFile` objects can exist

    std::vector<std::pair<std::string, TempFile>> files;

    void file(const std::string & id, const std::string & name) {
        files.push_back(std::pair<std::string, TempFile>(id, TempFile("", name)));
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
