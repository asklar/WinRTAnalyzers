# WinRT Analyzers

This project is a post-processing step that takes in WinMD files and runs analyzers.
These analyzers can emit diagnostics (e.g. to enforce best practices), as well as generate additional build artifacts that are consumed downstream.

## Implemented analyzers
### OOPCOMServer
This analyzer automatically generates an APPX fragment to expose a WinRT object as a COM server.
