# Building SpaceMonger

SpaceMonger is built using CMake and the Visual Studio 2022 MSVC toolchain, supporting native 32-bit (x86), 64-bit (x64), and ARM64 Windows targets.

## Prerequisites

- Visual Studio 2022 with the C++ desktop tools
- Visual Studio component: `C++ MFC for latest v143 build tools (x86 & x64)` (and optionally ARM64/ARM64EC for ARM64 builds)
- CMake 3.25 or newer

If configure fails with `MFC headers were not found` or the compiler cannot find `afxwin.h`, install the MFC component from Visual Studio Installer and rerun the configure command.

## Command Line

### 64-bit (x64) — Recommended

```powershell
cmake --preset vs2022-x64
cmake --build --preset vs2022-x64-debug
ctest --preset vs2022-x64-debug

# Optimized Release build:
cmake --build --preset vs2022-x64-release
ctest --preset vs2022-x64-release
```
The generated executable is written under `build/vs2022-x64/Release/SpaceMonger.exe`.

### 32-bit (Win32 / x86)

```powershell
cmake --preset vs2022-win32
cmake --build --preset vs2022-win32-release
ctest --preset vs2022-win32-release
```
The generated executable is written under `build/vs2022-win32/Release/SpaceMonger.exe`.

### ARM64 (Windows on ARM)

```powershell
cmake --preset vs2022-arm64
cmake --build --preset vs2022-arm64-release
```
The generated executable is written under `build/vs2022-arm64/Release/SpaceMonger.exe`.

## Visual Studio

Open the generated solution `build/vs2022-x64/SpaceMonger.sln` or `build/vs2022-win32/SpaceMonger.sln` after configuring with CMake.

Do not open `legacy/SpaceMonger.dsw` in modern Visual Studio as the primary workflow; it is the historical Visual C++ 6 workspace.

## VS Code

Open the folder and select your desired CMake configure preset (`vs2022-x64`, `vs2022-win32`, or `vs2022-arm64`) using the CMake Tools extension, or run the command-line presets from the integrated terminal.


## Validation coverage

CMake explicitly requires C++17. CI builds Debug and Release for Win32, x64,
and ARM64, and runs the nine CTest executables on Win32 and x64. ARM64 tests
require a compatible runtime and are not currently executed by CI.

Scanner regression tests create temporary directories, including an access-denied
fixture whose permissions are restored before cleanup. They cover missing and
empty roots, partial scans, cancellation, snapshot lifetime, and allocation failure.
Run with a bounded timeout when investigating hangs:

```powershell
ctest --preset vs2022-x64-debug --timeout 60
```

Automated checks do not replace interactive verification of scanning dialogs,
mixed-DPI rendering, and shell operations. Release builds use the static runtime
and are distributed as a single SpaceMonger.exe.
