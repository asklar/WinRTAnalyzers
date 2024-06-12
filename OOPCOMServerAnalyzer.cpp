#include <string_view>
#include <winmd_reader.h>
#include "Utility.h"
#include <sddl.h>

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
        winrt::hstring wClsidNoBraces(std::wstring_view(wClsid + 1, 36));


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
        GetFragment().AppendChild(extensions);
    }
}
