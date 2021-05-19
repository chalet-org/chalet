## chalet-debug

### Windows

---

Start with the clean msys2 install. The below version includes GCC 10.2.0.

- [MSYS2 (64-bit, 4/19/2021 stable)](https://repo.msys2.org/distrib/x86_64/msys2-x86_64-20210419.exe)

1. Don't update, for stability
2. Install the current stable GCC toolchain via:

```
pacman -S mingw-w64-x86_64-toolchain
```

3. Install [CMake](https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-win64-x64.msi) & [Git for Windows](https://github.com/git-for-windows/git/releases/download/v2.30.0.windows.1/Git-2.30.0-64-bit.exe)
4. Make sure CMake Is accessible from Path (add to System Environment Variables)
5. Run `bash ./build_all.sh` from the project root

### Windows (MSVC)

---

1. Install Visual Studio 2019
2. Install [CMake](https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-win64-x64.msi) & [Git for Windows](https://github.com/git-for-windows/git/releases/download/v2.30.0.windows.1/Git-2.30.0-64-bit.exe)
3. Make sure CMake Is accessible from Path (add to System Environment Variables)
4. Start "x64 Native Tools Command Prompt for VS 2019"
5. Run `.\build_all.bat` from the project root

### Linux

---

(Requires a compiler that supports C++17)

1. Install Git, CMake from your package manager
2. Make sure CMake is added to PATH
3. Run `bash ./build_all.sh` from the project root

### Mac

---

1. Install Command Line Tools & Homebrew
2. Install CMake (via Homebrew is easiest)
3. Run `bash ./build_all.sh` from the project root
