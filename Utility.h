#pragma once
#include <string>
#include <string_view>
#include <winmd_reader.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Data.Xml.Dom.h>

winmd::reader::CustomAttribute get_attribute(winmd::reader::TypeDef const& type, std::string_view attribute_name);
std::string_view get_attribute_string_value(winmd::reader::CustomAttribute const& attribute, std::string_view name);
std::string getWindowsWinMd();
void PrettyPrintNode(winrt::Windows::Data::Xml::Dom::IXmlNode const& node, std::wstringstream& output, int depth);

inline auto PV(std::wstring_view s) {
    return winrt::Windows::Foundation::PropertyValue::CreateString(s);
}

constexpr auto xmlns = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10";
constexpr auto xmlns_mp = L"http://schemas.microsoft.com/appx/2014/phone/manifest";
constexpr auto xmlns_com = L"http://schemas.microsoft.com/appx/manifest/com/windows10";
constexpr auto xmlns_uap = L"http://schemas.microsoft.com/appx/manifest/uap/windows10";
constexpr auto xmlns_uap3 = L"http://schemas.microsoft.com/appx/manifest/uap/windows10/3";
constexpr auto xmlns_rescap = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities";
constexpr auto xmlns_wincap = L"http://schemas.microsoft.com/appx/manifest/foundation/windows10/windowscapabilities";

winrt::Windows::Data::Xml::Dom::XmlElement GetFragment();
extern winrt::Windows::Data::Xml::Dom::XmlDocument xml;

void PrintAvailableAnalyzers(std::ostream& out);