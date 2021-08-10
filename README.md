## chalet-debug

---
### Windows

**Install**: 
* [CMake](https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-win64-x64.msi)
* [Git for Windows](https://github.com/git-for-windows/git/releases/download/v2.30.0.windows.1/Git-2.30.0-64-bit.exe)


**Visual Studio**
1. Run `./fetch_vendors.sh` from Git Bash. This will get Chalet dependencies
2. Make sure CMake is accessible from Path (add to System Environment Variables)
3. Start "x64 Native Tools Command Prompt for VS 20XX"
4. Run `.\build_all.bat` from the project root

**MSYS2 with MinGW**

1. Get [MSYS2](https://www.msys2.org/)
2. Install the current stable GCC toolchain via:

```
pacman -S mingw-w64-x86_64-toolchain
```

3. Install `cmake` via pacman, otherwise install it from the link above and make sure it's accessible from Path (add to System Environment Variables)
4. Run `bash ./build_all.sh` from the project root

---
### MacOS

1. Install Xcode or Command Line Tools
2. Install Homebrew
3. Install CMake (via Homebrew is easiest)
4. Run `bash ./build_all.sh` from the project root

---
### Linux

A compiler that supports C++17 is required.

1. Install `git` and `cmake` from your package manager, as well as `libunwind-dev`
2. Run `bash ./build_all.sh` from the project root
