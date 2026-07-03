# Building SpaceMonger

SpaceMonger is an old Win32 MFC application. The original Visual C++ 6
workspace files are still present, but the supported modern build path is CMake
with the 32-bit MSVC toolchain.

## Prerequisites

- Visual Studio 2022 with the C++ desktop tools
- Visual Studio component: `C++ MFC for latest v143 build tools (x86 & x64)`
- CMake 3.20 or newer

If configure fails with `MFC headers were not found` or the compiler cannot find
`afxwin.h`, install the MFC component from Visual Studio Installer and rerun the
configure command.

## Command Line

```powershell
cmake --preset vs2022-win32
cmake --build --preset vs2022-win32-debug
ctest --preset vs2022-win32-debug
```

For an optimized build:

```powershell
cmake --build --preset vs2022-win32-release
```

The generated executable is written under `build/vs2022-win32/<Config>/`.

## Visual Studio

Open `build/vs2022-win32/SpaceMonger.sln` after running:

```powershell
cmake --preset vs2022-win32
```

Do not open `SpaceMonger.dsw` in modern Visual Studio as the primary workflow;
it is the historical Visual C++ 6 workspace.

## VS Code

Open the folder and use the CMake Tools extension with the
`vs2022-win32` preset, or run the command-line steps from the integrated
terminal.

The checked-in VS Code tasks mirror the CMake commands:

- `CMake: configure VS 2022 Win32`
- `CMake: build Debug`
- `CMake: build Release`
- `CMake: test Debug`

The C/C++ extension configuration delegates IntelliSense setup to CMake Tools
so local MSVC and Windows SDK paths do not need to be committed.
