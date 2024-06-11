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

struct Options {
    std::wstring sdkVersion;
    std::string winMDPath;
};

Options* opts;

std::string getWindowsWinMd() {
    // The root location for Windows SDKs is stored in HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Kits\Installed Roots
    // under KitsRoot10
    // But it should be ok to check the default path for most cases

    const std::filesystem::path sdkRoot = LR"(C:\Program Files (x86)\Windows Kits\10\UnionMetadata)";
    if (!opts->sdkVersion.empty()) {
        return (sdkRoot / opts->sdkVersion / "Windows.winmd").string();
    }

    for (const auto& d : std::filesystem::directory_iterator(sdkRoot)) {
        auto dirPath = d.path();
        std::filesystem::path winmd = dirPath / "Windows.winmd";
        if (std::filesystem::exists(winmd)) {
            return winmd.string();
        }
    }

    throw std::invalid_argument("Couldn't find Windows.winmd");
}

winmd::reader::CustomAttribute get_attribute(winmd::reader::TypeDef const& type, std::string_view attribute_name)
{
    for (const auto& attribute : type.CustomAttribute())
    {
        if (attribute.TypeNamespaceAndName().second == attribute_name)
        {
            return attribute;
        }
    }

    return {};
}

std::string_view get_attribute_string_value(winmd::reader::CustomAttribute const& attribute, std::string_view name)
{
    auto sig = attribute.Value();
    for (const auto& [name_, value] : sig.NamedArgs())
    {
        if (name_ == name)
        {
            if (std::holds_alternative<winmd::reader::ElemSig>(value.value))
            {
                auto elem = std::get<winmd::reader::ElemSig>(value.value);
                if (std::holds_alternative<std::string_view>(elem.value))
                {
                    return std::get<std::string_view>(elem.value);
                }
            }
            return {};
        }
    }

    return {};
}

using Analyzer = void(*)(std::string_view name, winmd::reader::TypeDef const& def);
winrt::Windows::Data::Xml::Dom::XmlDocument xml;
winrt::Windows::Data::Xml::Dom::XmlElement fragment{ nullptr };


auto PV(std::wstring_view s) {
    return winrt::Windows::Foundation::PropertyValue::CreateString(s);
}

constexpr auto xmlns = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10";
constexpr auto xmlns_mp = L"http://schemas.microsoft.com/appx/2014/phone/manifest";
constexpr auto xmlns_com = L"http://schemas.microsoft.com/appx/manifest/com/windows10";
constexpr auto xmlns_uap = L"http://schemas.microsoft.com/appx/manifest/uap/windows10";
constexpr auto xmlns_uap3 = L"http://schemas.microsoft.com/appx/manifest/uap/windows10/3";
constexpr auto xmlns_rescap = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities";
constexpr auto xmlns_wincap = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10/windowscapabilities";

std::wstring Indent(int depth) {
    return std::wstring(depth * 4, L' ');
}

void PrettyPrintNode(winrt::Windows::Data::Xml::Dom::IXmlNode const& node, std::wstringstream& output, int depth) {
    using namespace winrt::Windows::Data::Xml::Dom;
    if (node.NodeType() == NodeType::ElementNode) {
        auto element = node.as<XmlElement>();
        output << Indent(depth) << L"<" << element.TagName().c_str();
        for (auto attribute : element.Attributes()) {
            auto value = winrt::unbox_value<winrt::hstring>(attribute.NodeValue());
            output << L" " << attribute.NodeName().c_str() << L"=\"" << value.c_str() << L"\"";
        }
        if (!node.HasChildNodes()) {
            output << L"/>" << std::endl;
        }
        else {
            output << L">" << std::endl;
            for (auto child : node.ChildNodes()) {
                PrettyPrintNode(child, output, depth + 1);
            }
            output << Indent(depth) << L"</" << element.TagName().c_str() << L">" << std::endl;
        }
    }
    else if (node.NodeType() == NodeType::TextNode) {
        output << Indent(depth) << node.InnerText().c_str() << std::endl;
    }
}

void OOPCOMServerAnalyzer(std::string_view name, winmd::reader::TypeDef const& def)
{
    if (auto ComServerAttribute = get_attribute(def, "ComServerAttribute"))
    {
        auto clsid = get_attribute_string_value(ComServerAttribute, "Clsid");
        if (clsid.empty())
        {
            throw std::runtime_error("Clsid not found");
        }
        else if (clsid[0] != '{' || clsid.size() != 38 || clsid[37] != '}')
        {
            throw std::runtime_error("Invalid CLSID: " + std::string{ clsid });
        }
        wchar_t wClsid[40]{};
        if (MultiByteToWideChar(CP_UTF8, 0, clsid.data(), static_cast<int>(clsid.size()), wClsid, 40) == 0)
        {
            throw std::runtime_error("MultiByteToWideChar failed");
        }

        CLSID validClsid{};
        if (CLSIDFromString(wClsid, &validClsid) != S_OK)
        {
            throw std::runtime_error("Invalid CLSID: " + std::string{ clsid });
        }
        
        // print back the CLSID without the braces to wClsid. Note StringFromGUID2 adds the braces so we do this by hand
        winrt::hstring wClsidNoBraces(std::wstring_view(wClsid + 1, wClsid + 37));


        auto executable = get_attribute_string_value(ComServerAttribute, "Executable");
        if (executable.empty())
        {
            throw std::runtime_error("Executable not found");
        }

        auto extensions = xml.CreateElementNS(PV(xmlns_uap), L"uap:Extensions");
        auto extension = xml.CreateElementNS(PV(xmlns_com), L"com:Extension");
        extension.SetAttribute(L"Category", L"windows.comServer");
        auto comServer = xml.CreateElementNS(PV(xmlns_com), L"com:ComServer");
        auto exeServer = xml.CreateElementNS(PV(xmlns_com), L"com:ExeServer");
        exeServer.SetAttribute(L"Executable", winrt::to_hstring(executable));
        winrt::hstring displayName;
        if (auto displayNameAttr = get_attribute(def, "DisplayNameAttribute"))
        {
            auto displayNameA = get_attribute_string_value(displayNameAttr, "DisplayName");
            if (!displayNameA.empty())
            {
                displayName = winrt::to_hstring(displayNameA);
                exeServer.SetAttribute(L"DisplayName", displayName);
            }
        }

        if (auto launchAndActivationPermissionAttr = get_attribute(def, "LaunchAndActivationPermissionAttribute"))
        {
            auto SDDL = get_attribute_string_value(launchAndActivationPermissionAttr, "SDDL");
            if (!SDDL.empty())
            {
                PSECURITY_DESCRIPTOR sd{};
                if (ConvertStringSecurityDescriptorToSecurityDescriptorA(SDDL.data(), SDDL_REVISION_1, &sd, nullptr) == 0)
                {
                    throw std::runtime_error("ConvertStringSecurityDescriptorToSecurityDescriptorW failed");
                }
                LocalFree(sd);

                exeServer.SetAttribute(L"LaunchAndActivationPermission", winrt::to_hstring(SDDL));
            }
        }

        auto cls = xml.CreateElementNS(PV(xmlns_com), L"com:Class");
        cls.SetAttribute(L"Id", wClsidNoBraces);

        if (!displayName.empty())
        {
            cls.SetAttribute(L"DisplayName", displayName);
        }

        exeServer.AppendChild(cls);
        comServer.AppendChild(exeServer);
        extension.AppendChild(comServer);
        extensions.AppendChild(extension);
        fragment.AppendChild(extensions);
    }
}


std::array<Analyzer, 1> analyzers = {
    &OOPCOMServerAnalyzer,
};

int main()
{
    opts = new Options();
    opts->winMDPath = R"(E:\source\ConsoleApplication1\ConsoleA.b1bfaa8b\x64\Debug\Merged\ConsoleApplication1.winmd)";
    std::vector<std::string> files = {
        getWindowsWinMd(),
        opts->winMDPath,
    };


    xml = winrt::Windows::Data::Xml::Dom::XmlDocument();
    xml.CreateProcessingInstruction(L"xml", L"version='1.0' encoding='utf-8'");
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

    auto cache = std::make_unique<winmd::reader::cache>(files);

    for (const auto& [ns, members] : cache->namespaces())
    {
        for (const auto& [name, def] : members.types)
        {
            for (const auto& analyzer : analyzers)
            {
                try
                {
                    analyzer(name, def);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error analyzing type " << name << ": " << e.what() << std::endl;
                    return -1;
                }
            }

        }
    }

    std::wstringstream pretty;
    PrettyPrintNode(fragment, pretty, 0);
    std::wcout << pretty.str() << std::endl;

}
