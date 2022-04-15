## Chalet

A compiler with C++20 is required to build Chalet from source. The following versions are known to work:

MSVC 19.30.30706
Apple Clang 13.1.6
GCC 11.2.0

---
### Windows

**Install**: 
* [CMake](https://cmake.org/download)
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

1. Install Xcode or Command Line Tools
2. Install CMake (via Homebrew is easiest)
3. Run `bash ./build_all.sh` from the project root

---
### Linux

A compiler that supports C++17 is required.

1. Install `git` and `cmake` from your package manager
2. Run `bash ./build_all.sh` from the project root
