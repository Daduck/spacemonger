# Modernization Backlog

SpaceMonger is now buildable with VS2022/CMake, so the useful work is
incremental modernization rather than a full rewrite.

## Next

- [x] Replace stale VS Code tasks with CMake configure/build/test tasks.
- [x] Remove local absolute compiler and SDK paths from VS Code C/C++ settings.
- [x] Document the VS Code workflow in `BUILDING.md`.
- [x] Fix GDI / HICON leak in `CDriveDialog` with proper `CDriveInfo` RAII destruction.

## Filesystem Scanning

- [x] Move scan traversal away from ANSI-only Win32 calls.
- [x] Add long-path-safe path construction before touching recursive scanning.
- [x] Add tests around path joining and file-size accounting before changing scan
      behavior.
- [x] Review reparse-point handling for modern Windows symlinks, mount points,
      and cloud placeholders.
- [x] **Multi-Threaded / Asynchronous Scanning**
      *Background:* `CFolder::LoadFolder` currently runs synchronously on the main thread, intermittently pumping messages via `::PeekMessage` to keep the UI from completely freezing. On modern multi-core systems and high-IOPS NVMe SSDs, single-threaded synchronous directory enumeration leaves significant I/O throughput untapped. Decoupling directory walking into a background worker thread pool (or task queue with work-stealing) with thread-safe tree aggregation will cut scan times significantly on modern drives and keep the UI completely responsive.
- [x] **Live Progressive Treemap Rendering During Scan**
      *Background:* Render live snapshots of the treemap at 10 FPS directly behind the classic "Scanning Disk..." progress dialog as background worker threads discover files and folders, providing dynamic visual feedback without degrading NVMe/SSD scan throughput.

## Code Health & Architecture

- [x] **Extract Treemap Layout Engine into a Pure Module**
      *Background:* Treemap coordinate partitioning (`BuildFolderLayout`, `SizeFolders`) is currently embedded directly inside `CFolderView`, interleaved with MFC device context drawing. Extracting the spatial partitioning algorithm into an independent, non-UI module (following the pattern of `DiskUsage`) will allow headless automated unit testing for tricky geometries, degenerate aspect ratios, and zero-byte files without requiring an initialized Win32 window.
- [x] Replace fixed-size buffers and unsafe formatting calls in narrow passes.
- [ ] Decide whether unused `CFolder` mutation methods (`DelFile`, `RenameFile`, `FindFile`) should be implemented or removed.
- [ ] **Full Unicode Migration (`_UNICODE` / `UNICODE`)**
      *Background:* The app target currently builds with `_MBCS`. Although the internal filesystem scanner was upgraded to wide Win32 APIs (`WIN32_FIND_DATAW`, `std::wstring`), the MFC application shell, dialogs, settings, and tooltips still operate in ANSI mode with repeated `PathUtil::WideToAnsi` and `AnsiToWide` conversions. Migrating the entire target to `UNICODE` / `_UNICODE` eliminates conversion overhead, ensures native `CStringW` handling throughout, and prevents character corruption when viewing non-ASCII / foreign file paths.
- [x] Investigate C++ exceptions occasionally thrown in `FolderView` layout calculation for degenerate aspect ratios.

## Performance Optimizations

- [x] Increase `CFolder` initial array capacity (from `max = 2` to `16` or `32`) to eliminate thousands of `malloc`/`memcpy` reallocations.
- [x] Implement an Arena Allocator (Memory Pool) for filenames to prevent tiny heap fragmentation during large drive scans.
- [x] Use adaptive sorting in `CFolder::Finalize` (e.g., `std::sort` for small folders instead of an 8-pass Radix sort).
- [x] Store internal strings as `wchar_t*` instead of `char*` to eliminate `PathUtil::WideToAnsi` conversion overhead during scanning.
- [ ] **Modernize `CFolder` Data Layout (Struct of Arrays -> Contiguous Structures)**
      *Background:* `CFolder` currently manages five separate heap-allocated pointer arrays (`names`, `children`, `sizes`, `actualsizes`, `times`) through manual `malloc`/`free` and reallocation in `MoreEntries()`. Refactoring this into a single contiguous struct (e.g., `struct FolderEntry`) or an arena-backed contiguous array improves CPU cache locality during treemap layout traversal and sorting passes, while replacing error-prone manual memory management with clean RAII semantics.
- [x] Add a line-ending policy with `.gitattributes`.

## Platform & UI Modernization

- [x] **64-bit (x64) and ARM64 Build Support**
      *Background:* `CMakeLists.txt` currently enforces 32-bit compilation (`CMAKE_SIZEOF_VOID_P EQUAL 4`). On modern Windows installations with high-capacity drives containing millions of files, a 32-bit process is constrained by the 2–3 GB user-mode virtual address space. Updating CMake configurations, pointer/integer conversions, and CI to support `x64` and `ARM64` builds provides full memory scalability and native execution on Windows on ARM hardware (e.g. Snapdragon X / Surface).
- [x] **High-DPI Support (Per-Monitor V2) & Classic 3D Character Preservation**
      *Background:* SpaceMonger retains its authentic, beloved 1990s retro 3D beveled toolbar buttons and crisp pixel typography while adding modern `PerMonitorV2` DPI awareness, long path support, and flicker-free button state updates.
- [ ] **Dark Mode Theme**: Optional Dark Mode palette for the treemap background, borders, and tooltip windows.
- [ ] **Custom Folder & UNC Path Scanning**: Add modern folder picker (`IFileOpenDialog` with `FOS_PICKFOLDERS`) to support scanning specific folders, mount points, and network shares.
- [ ] **Filters and Exclusions (Low Priority)**: Add the ability to ignore certain paths (e.g., `.git`, `node_modules`) or filter by file extensions.

## Project Hygiene

- [x] Decide whether the original VC6 workspace files remain historical
      artifacts or should move under a legacy folder.
- [x] Add a CI workflow once the required Windows/MFC environment is confirmed.

## Architectural Decisions

- **Keep Incremental C++ Modernization (2026-07-03):** Decided to stick with C++ and MFC modernization instead of doing a full rewrite (e.g. in Rust, C#). SpaceMonger's strength is its tiny executable size (< 1MB), lack of dependencies, and extreme snappiness. Upgrading scanning to wide/Unicode APIs solves the primary compatibility issues with modern OS installations (long paths, reparse points) while retaining these benefits.
- **Strict Single-File Executable Distribution (2026-08-13):** Distribution must strictly remain a single standalone `.exe` file (`SpaceMonger.exe`), with zero installer and no required `.zip` wrapping or accompanying files. Release notes and documentation live on the official website (https://andedammen.dk/spacemonger) and GitHub Releases rather than bundled text files.
- **Preserve Authentic 3D Retro Aesthetic (2026-08-13):** Do not impose flat, modern ComCtl32 v6 themes that strip the classic 3D embossed button borders or introduce blue hover boxes. SpaceMonger's charm lies in its clean, distinct 1990s aesthetic (beveled 3D buttons, crisp "Small Fonts" bitmap font, solid classic UI).

