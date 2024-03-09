## Build from Source

A C++17 compiler is required to build Chalet from source. Chalet is developed on various compiler versions, but the following versions are currently used during development:

MSVC `>= 19.30`  
Apple Clang `>= 13.x`  
GCC / MinGW `>= 11.x`

C++17 is targeted instead of C++20 for now to ensure a wide gamut of supported operating systems.

---
### Windows

**Install**: 

* [CMake](https://cmake.org/download) `>= 3.16`
* [Git for Windows](https://gitforwindows.org)
* [Visual Studio 2022](https://visualstudio.microsoft.com)

1. Make sure CMake is accessible from Path (add to System Environment Variables)
2. Run `.\build_all.bat` from the project root
3. Open the `build\msvc_debug\Chalet.sln` in Visual Studio if necessary

---
### MacOS

**Install**: 

* [CMake](https://cmake.org/download) `>= 3.16`

1. Install Xcode or Command Line Tools
2. Install CMake if it's not already
3. Run `bash ./build_all.sh` from the project root

---
### Linux

**Ubuntu / Debian**

A GCC version that supports C++17 is required. (`>= 7.3`)

1. Install the following packages:

```bash
sudo apt install git cmake ninja-build uuid-dev zip debhelper
```

2. Run `bash ./build_all.sh` from the project root

**Arch Linux / Manjaro**

A GCC version that supports C++17 is required. (`>= 7.3`)

1. Install the following packages:

```bash
sudo pacman -S git cmake ninja-build zip
```

2. Install `debhelper` from the AUR
3. Run `bash ./build_all.sh` from the project root
