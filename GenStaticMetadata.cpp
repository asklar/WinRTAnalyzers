// GenStaticMetadata.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <winmd_reader.h>
#include <sddl.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.data.xml.dom.h>
#include <wil/win32_helpers.h>

#include "Options.h"
#include "Utility.h"
Options* opts;

struct Analyzer {
    std::string_view name;
    void(*invoke)(std::string_view name, winmd::reader::TypeDef const& def);
};

winrt::Windows::Data::Xml::Dom::XmlDocument xml;
winrt::Windows::Data::Xml::Dom::XmlElement fragment{ nullptr };

winrt::Windows::Data::Xml::Dom::XmlElement GetFragment()
{
    if (!fragment)
    {
        fragment = xml.CreateElement(L"Fragment");

        fragment.SetAttribute(L"xmlns", xmlns);
        fragment.SetAttribute(L"xmlns:mp", xmlns_mp);
        fragment.SetAttribute(L"xmlns:com", xmlns_com);
        fragment.SetAttribute(L"xmlns:uap", xmlns_uap);
        fragment.SetAttribute(L"xmlns:uap3", xmlns_uap3);
        fragment.SetAttribute(L"xmlns:rescap", xmlns_rescap);
        fragment.SetAttribute(L"xmlns:wincap", xmlns_wincap);
        fragment.SetAttribute(L"IgnorableNamespaces", L"mp uap");

        xml.AppendChild(fragment);
    }
    return fragment;
}

#define ANALYZER(name) Analyzer { #name, name ## Analyzer }

void OOPCOMServerAnalyzer(std::string_view name, winmd::reader::TypeDef const& def);

std::array<Analyzer, 1> all_analyzers = {
    ANALYZER(OOPCOMServer),
};

int main(int argc, char** argv)
{
    opts = new Options(args_view(argc, argv));
    auto exePath = wil::GetModuleFileNameW();
    auto folder = std::filesystem::path(exePath.get()).parent_path();
    opts->winMDPath = (folder / "ConsoleApplication1.winmd").string();
    std::vector<std::string> files = {
        getWindowsWinMd(),
        opts->winMDPath,
    };

    xml = winrt::Windows::Data::Xml::Dom::XmlDocument();
    xml.CreateProcessingInstruction(L"xml", L"version='1.0' encoding='utf-8'");

    auto cache = std::make_unique<winmd::reader::cache>(files);

    for (const auto& [ns, members] : cache->namespaces())
    {
        for (const auto& [name, def] : members.types)
        {
            for (const auto& analyzerName : opts->enabled_analyzers)
            {
                const auto& analyzer = std::find_if(all_analyzers.begin(), all_analyzers.end(), [&](const Analyzer& a) { return a.name == analyzerName; });
                if (analyzer == all_analyzers.end())
                {
                    std::cerr << "Unknown analyzer: " << analyzerName << std::endl;
                    std::cerr << "Available analyzers: ";
                    for (const auto& a : all_analyzers)
                    {
                        std::cerr << a.name << " ";
                    }
                    return -1;
                }
                try
                {
                    analyzer->invoke(name, def);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error analyzing type " << name << ": " << e.what() << std::endl;
                    return -1;
                }
            }

        }
    }

    if (fragment)
    {
        std::wstringstream pretty;
        PrettyPrintNode(fragment, pretty, 0);
        std::wcout << pretty.str() << std::endl;
    }
}
