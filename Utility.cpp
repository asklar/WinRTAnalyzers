#include "Utility.h"
#include "Options.h"
#include <winrt/Windows.Foundation.Collections.h>

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
