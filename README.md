# WinRT Analyzers

This project is a post-processing step that takes in WinMD files and runs analyzers.
These analyzers can emit diagnostics (e.g. to enforce best practices), as well as generate additional build artifacts that are consumed downstream.

## Implemented analyzers
### OOPCOMServer
This analyzer automatically generates an APPX fragment to expose a WinRT object as a COM server.

Example usage:
```csharp
namespace ConsoleApplication 1 {
    [attributeusage(target_runtimeclass)]
    attribute ComServerAttribute
    {
        String Clsid;
        String Executable;
    }

    [attributeusage(target_runtimeclass)]
    attribute DisplayNameAttribute
    {
        String DisplayName;
    }

    [attributeusage(target_runtimeclass)]
    attribute LaunchAndActivationPermissionAttribute
    {
        String SDDL;
    }

    [ComServer("{C0A3D611-3E3D-4D0D-8D3D-3D3D3D3D3D3D}", "ConsoleApplication1.exe")]
    [DisplayName("My COM Server")]
    [LaunchAndActivationPermission("D:P(A;;GA;;;SY)(A;;GA;;;BA)")]
    runtimeclass MyCOMServer
    {
        MyCOMServer();
        void DoSomething();
    }
}
```