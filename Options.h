#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "ArgsView.h"
#include "Utility.h"

struct Options {
    std::wstring sdkVersion;
    std::string winMDPath;
    std::vector<std::string> enabled_analyzers;
    std::string outputFolder;
private:
    args_view<char> m_args;

    void help() {
        printf("Usage: %s [options]\n", m_args[0].data());
        printf("Options:\n");
        printf("  -sdk <version>    Windows SDK version\n");
        printf("  -winmd <path>     Path to winmd file\n");
        printf("  -analyzer <name>  Analyzer to run\n");
        printf("  -list             Lists available analyzers\n");
        printf("  -output <path>    Output folder\n");
        printf("  -help             Display this information\n");
    }
public:
    Options(const args_view<char>& args) : m_args(args) {
        for (int i = 1; i < args.argc; ++i) {
            if (args[i] == "-sdk") {
                auto v = args[++i];
                sdkVersion = std::wstring(v.begin(), v.end());
            }
            else if (args[i] == "-winmd") {
                winMDPath = args[++i];
            }
            else if (args[i] == "-analyzer") {
                enabled_analyzers.push_back(std::string{ args[++i] });
            }
            else if (args[i] == "-help") {
                help();
                exit(0);
            }
            else if (args[i] == "-list") {
                PrintAvailableAnalyzers(std::cout);
                exit(0);
            }
            else if (args[i] == "-output") {
                outputFolder = args[++i];
            }
            else {
                printf("Unknown option: %s\n", args[i].data());
                help();
                exit(1);
            }
        }
//#ifndef _DEBUG
//        if (winMDPath.empty()) {
//            printf("Missing -winmd option\n");
//            help();
//            exit(1);
//        }
//#endif
    }
};


extern Options* opts;