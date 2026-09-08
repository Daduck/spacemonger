# Modernization Backlog

SpaceMonger is now buildable with VS2022/CMake, so the useful work is
incremental modernization rather than a full rewrite.

## Next

- [ ] Complete the Unicode UI migration described below.
- [ ] Benchmark live-layout mutex hold time and scan throughput before implementing incremental size propagation.
- [ ] Resolve existing x64 conversion warnings in DriveDialog.cpp and Freedoc.cpp.
- [ ] Verify failed/partial-scan dialogs, cancellation, and mixed-DPI behavior interactively.

## Recently Completed

- [x] Fix live snapshot ownership on allocation failure and exception paths; add allocation-failure regression coverage.
- [x] Make allocated-size API initialization safe for concurrent scan workers.
- [x] Report failed roots and partial scans, including access-denied directories and depth-limit truncation.
- [x] Reconcile current documentation and explicitly select C++17 in CMake.
- [x] Replace stale VS Code tasks with CMake configure/build/test tasks.
- [x] Remove local absolute compiler and SDK paths from VS Code C/C++ settings.
- [x] Document the VS Code workflow in BUILDING.md.
- [x] Fix GDI / HICON leak in CDriveDialog with proper CDriveInfo RAII destruction.

## Filesystem Scanning

- [x] Move scan traversal away from ANSI-only Win32 calls.
- [x] Add long-path-safe path construction before touching recursive scanning.
- [x] Add tests around path joining and file-size accounting before changing scan
      behavior.
- [x] Review reparse-point handling for modern Windows symlinks, mount points,
      and cloud placeholders.
- [x] **Multi-Threaded / Asynchronous Scanning**
      Implemented in AsyncScanEngine with background directory workers and a UI message loop. Throughput depends on the filesystem and live-layout work; measure before claiming a speedup.
- [x] **Live Progressive Treemap Rendering During Scan**
      Implemented with a 250 ms minimum interval and change detection. Snapshots own their copied names; layout work still holds the tree mutex.

## Code Health & Architecture

- [x] **Extract Treemap Layout Engine into a Pure Module**
      Implemented in TreemapLayout.cpp with headless layout and hit-testing regression coverage.
- [x] Replace fixed-size buffers and unsafe formatting calls in narrow passes.
- [ ] Decide whether unused `CFolder` mutation methods (`DelFile`, `RenameFile`, `FindFile`) should be implemented or removed.
- [ ] **Full Unicode Migration (`_UNICODE` / `UNICODE`)**
      *Background:* The app target currently builds with `_MBCS`. Although the internal filesystem scanner was upgraded to wide Win32 APIs (`WIN32_FIND_DATAW`, `std::wstring`), the MFC application shell, dialogs, settings, and tooltips still operate in ANSI mode with repeated `PathUtil::WideToAnsi` and `AnsiToWide` conversions. Migrating the entire target to `UNICODE` / `_UNICODE` eliminates conversion overhead, ensures native `CStringW` handling throughout, and prevents character corruption when viewing non-ASCII / foreign file paths.
- [x] Investigate C++ exceptions occasionally thrown in `FolderView` layout calculation for degenerate aspect ratios.

## Performance Optimizations

- [x] Allocate CFolder entry arrays lazily, starting at eight entries and doubling as needed.
- [x] Implement an Arena Allocator (Memory Pool) for filenames to prevent tiny heap fragmentation during large drive scans.
- [x] Use adaptive sorting in `CFolder::Finalize` (e.g., `std::sort` for small folders instead of an 8-pass Radix sort).
- [x] Store internal strings as `wchar_t*` instead of `char*` to eliminate `PathUtil::WideToAnsi` conversion overhead during scanning.
- [ ] **Incremental Live-Scan Size Propagation (replace `ComputeLiveSizes` full-tree walk)**
      *Background:* `AsyncScanEngine::GenerateLiveLayout` calls `ComputeLiveSizes`, which recursively recomputes folder sizes for the entire partially scanned tree on every 250 ms render tick while holding `m_treeMutex` — an O(N) walk that blocks all scan workers for its duration. Fine for typical disks at 4 Hz, but on multi-million-file scans the walk grows linearly and increasingly stalls scanning. The fix is incremental propagation: have workers bubble size deltas up the parent chain (atomics or per-folder accumulation) as files are discovered, so the render tick only reads already-current sizes instead of recomputing them from scratch.
- [ ] **Modernize `CFolder` Data Layout (Struct of Arrays -> Contiguous Structures)**
      *Background:* `CFolder` currently manages five separate heap-allocated pointer arrays (`names`, `children`, `sizes`, `actualsizes`, `times`) through manual `malloc`/`free` and reallocation in `MoreEntries()`. Refactoring this into a single contiguous struct (e.g., `struct FolderEntry`) or an arena-backed contiguous array improves CPU cache locality during treemap layout traversal and sorting passes, while replacing error-prone manual memory management with clean RAII semantics.
- [x] Add a line-ending policy with `.gitattributes`.

## Platform & UI Modernization

- [x] **64-bit (x64) and ARM64 Build Support**
      Implemented Win32, x64, and ARM64 build presets and CI builds. CI executes tests on x86/x64; ARM64 currently has build coverage only.
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

