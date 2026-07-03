# Modernization Backlog

SpaceMonger is now buildable with VS2022/CMake, so the useful work is
incremental modernization rather than a full rewrite.

## Next

- [x] Replace stale VS Code tasks with CMake configure/build/test tasks.
- [x] Remove local absolute compiler and SDK paths from VS Code C/C++ settings.
- [x] Document the VS Code workflow in `BUILDING.md`.

## Filesystem Scanning

- [x] Move scan traversal away from ANSI-only Win32 calls.
- [x] Add long-path-safe path construction before touching recursive scanning.
- [x] Add tests around path joining and file-size accounting before changing scan
      behavior.
- [x] Review reparse-point handling for modern Windows symlinks, mount points,
      and cloud placeholders.


## Code Health

- [ ] Extract more non-UI logic into testable modules, following `DiskUsage`.
- [x] Replace fixed-size buffers and unsafe formatting calls in narrow passes.
- [ ] Decide whether unused `CFolder` mutation methods should be implemented or
      removed.

## Performance Optimizations

- [x] Increase `CFolder` initial array capacity (from `max = 2` to `16` or `32`) to eliminate thousands of `malloc`/`memcpy` reallocations.
- [ ] Implement an Arena Allocator (Memory Pool) for filenames to prevent tiny heap fragmentation during large drive scans.
- [ ] Use adaptive sorting in `CFolder::Finalize` (e.g., `std::sort` for small folders instead of an 8-pass Radix sort).
- [ ] Store internal strings as `wchar_t*` instead of `char*` to eliminate `PathUtil::WideToAnsi` conversion overhead during scanning.
- [x] Add a line-ending policy with `.gitattributes`.

## Project Hygiene

- [x] Decide whether the original VC6 workspace files remain historical
      artifacts or should move under a legacy folder.
- [x] Add a CI workflow once the required Windows/MFC environment is confirmed.

## Architectural Decisions

- **Keep Incremental C++ Modernization (2026-07-03):** Decided to stick with C++ and MFC modernization instead of doing a full rewrite (e.g. in Rust, C#). SpaceMonger's strength is its tiny executable size (< 1MB), lack of dependencies, and extreme snappiness. Upgrading scanning to wide/Unicode APIs solves the primary compatibility issues with modern OS installations (long paths, reparse points) while retaining these benefits.
