# molumes

## Setup on Windows

### Prerequisites

- CMake: https://cmake.org/
- Git: https://tortoisegit.org/
- Microsoft Visual Studio 2015 or 2017 (2017 is recommended as it offers CMake integration)

### Setup

- Open a shell and run ./fetch-libs.cmd to download all dependencies.
- Run ./build-libs.cmd to build the dependencies.
- Run ./configure.cmd to create the Visual Studio solution files (only necessary for Visual Studion versions prior to 2017).
- Open ./build/molumes.sln in Visual Studio (only necessary for Visual Studion versions prior to 2017, in VS 2017 the CMakeFiles.txt can be opened directly).
