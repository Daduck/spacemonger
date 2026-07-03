# Modernization Backlog

SpaceMonger is now buildable with VS2022/CMake, so the useful work is
incremental modernization rather than a full rewrite.

## Next

- [x] Replace stale VS Code tasks with CMake configure/build/test tasks.
- [x] Remove local absolute compiler and SDK paths from VS Code C/C++ settings.
- [x] Document the VS Code workflow in `BUILDING.md`.

## Filesystem Scanning

- [ ] Move scan traversal away from ANSI-only Win32 calls.
- [ ] Add long-path-safe path construction before touching recursive scanning.
- [ ] Add tests around path joining and file-size accounting before changing scan
      behavior.
- [ ] Review reparse-point handling for modern Windows symlinks, mount points,
      and cloud placeholders.

## Code Health

- [ ] Extract more non-UI logic into testable modules, following `DiskUsage`.
- [ ] Replace fixed-size buffers and unsafe formatting calls in narrow passes.
- [ ] Decide whether unused `CFolder` mutation methods should be implemented or
      removed.
- [ ] Add a line-ending policy with `.gitattributes`.

## Project Hygiene

- [ ] Decide whether the original VC6 workspace files remain historical
      artifacts or should move under a legacy folder.
- [ ] Add a CI workflow once the required Windows/MFC environment is confirmed.
