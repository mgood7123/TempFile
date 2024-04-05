#include <stdio.h>
#include <vector>
#include <utility>

int main() {
    std::vector<std::pair<const char*, std::pair<const char*, const char*>>> header = {
        {
            "",
            {
                "",
                "return construct("", \"\", \"\", TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, false);"
            }
        },
        {
            "const std::string & dir",
            {
                "construct(dir);",
                "return construct(dir, \"\", \"\", TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, false);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix",
            {
                "construct(dir, template_prefix);",
                "return construct(dir, template_prefix, \"\", TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, false);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, int open_mode",
            {
                "construct(dir, template_prefix, open_mode);",
                "return construct(dir, template_prefix, \"\", open_mode, false);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, bool log_create_close",
            {
                "construct(dir, template_prefix, log_create_close);",
                "return construct(dir, template_prefix, \"\", TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, log_create_close);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, int open_mode, bool log_create_close",
            {
                "construct(dir, template_prefix, open_mode, log_create_close);",
                "return construct(dir, template_prefix, \"\", open_mode, log_create_close);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, char * template_suffix",
            {
                "construct(dir, template_prefix, std::string(template_suffix));",
                "return construct(dir, template_prefix, std::string(template_suffix));"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, const char * template_suffix",
            {
                "construct(dir, template_prefix, std::string(template_suffix));",
                "return construct(dir, template_prefix, std::string(template_suffix));"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, const std::string & template_suffix",
            {
                "construct(dir, template_prefix, template_suffix);",
                "return construct(dir, template_prefix, template_suffix, TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, false);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode",
            {
                "construct(dir, template_prefix, template_suffix, open_mode);",
                "return construct(dir, template_prefix, template_suffix, open_mode, false);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, bool log_create_close",
            {
                "construct(dir, template_prefix, template_suffix, log_create_close);",
                "return construct(dir, template_prefix, template_suffix, TEMP_FILE_OPEN_MODE_READ|TEMP_FILE_OPEN_MODE_WRITE, log_create_close);"
            }
        },
        {
            "const std::string & dir, const std::string & template_prefix, const std::string & template_suffix, int open_mode, bool log_create_close",
            {
                "construct(dir, template_prefix, template_suffix, open_mode, log_create_close);",
                "if (dir.length() == 0) {\n        return construct(TempDir(), template_prefix, template_suffix, open_mode, log_create_close);\n    }"
            }
        }
    };

    printf("class TempFileFILE {\n");
    for (auto& p : header) {
        printf("    TempFileFILE(%s);\n", p.first);
    }
    printf("\n");
    for (auto& p : header) {
        printf("    bool construct(%s);\n", p.first);
    }
    printf("}\n");
    printf("\n");

    for (auto& p : header) {
        printf("TempFileFILE::TempFileFILE(%s) {\n", p.first);
        printf("    data = std::make_shared<CleanUp>();\n");
        printf("    %s\n", p.second.first);
        printf("}\n");
    }
    printf("\n");
    for (auto& p : header) {
        printf("bool TempFileFILE::construct(%s) {\n", p.first);
        printf("    %s\n", p.second.second);
        printf("}\n");
    }
    printf("\n");

    return 0;
}
