#pragma once
#include <string>
#include <vector>
#include "ArgsView.h"

struct Options {
    std::wstring sdkVersion;
    std::string winMDPath;
    std::vector<std::string> enabled_analyzers;

    Options(const args_view<char>& args) {
        for (int i = 1; i < args.argc; ++i) {
            if (args[i] == "-sdk") {
                sdkVersion = std::wstring(args[++i], args[i] + strlen(args[i]));
            }
            else if (args[i] == "-winmd") {
                winMDPath = args[++i];
            }
            else if (args[i] == "-analyzer") {
                enabled_analyzers.push_back(args[++i]);
            }
        }
    }
};


extern Options* opts;