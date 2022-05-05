
![Chalet logo](chalet-banner.jpg)

<p align="center">A modern, cross-platform project format and build system for C/C++ focused on readability and interoperability.</p>

[![Github Releases](https://img.shields.io/github/release/chalet-org/chalet.svg?style=flat&color=orange)](https://github.com/chalet-org/chalet/releases)
[![License](https://img.shields.io/github/license/chalet-org/chalet.svg?style=flat)](https://github.com/chalet-org/chalet/blob/main/LICENSE.txt)

<br />
<br />

A compiler with C++17 is required to build Chalet from source. Chalet is developed on various compiler versions, but the following versions are currently used during development:

MSVC `>= 19.30`  
Apple Clang `>= 13.x`  
GCC / MinGW `>= 11.x`

---
### Windows

**Install**: 

* [CMake](https://cmake.org/download) `>= 3.16`
* [Git for Windows](https://gitforwindows.org)


**Visual Studio**

1. Run `./fetch_externals.sh` from Git Bash. This will get Chalet dependencies
2. Make sure CMake is accessible from Path (add to System Environment Variables)
3. Start "x64 Native Tools Command Prompt for VS 20XX"
4. Run `.\build_all.bat` from the project root

**MSYS2 with MinGW**

1. Get [MSYS2](https://www.msys2.org/)
2. Install the current stable GCC toolchain via:

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

3. Install `cmake` via pacman, otherwise install it from the link above and make sure it's accessible from Path (add to System Environment Variables)
4. Run `bash ./build_all.sh` from the project root

---
### MacOS

**Install**: 

* [CMake](https://cmake.org/download) `>= 3.16`

1. Install Xcode or Command Line Tools
2. Install CMake if it's not already
3. Run `bash ./build_all.sh` from the project root

---
### Linux

**Install**: 

A GCC version that supports C++17 is required. (`>= 7.3`)

1. Install `git` and `cmake` from your package manager
2. Run `bash ./build_all.sh` from the project root
