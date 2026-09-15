# SpaceMonger

SpaceMonger is a native Windows disk-space visualizer with a classic MFC
interface, background scanning, and a live treemap.

Download the standalone executable from
[the release website](https://andedammen.dk/spacemonger).
No installer or companion files are required.

- [Build and test instructions](BUILDING.md): Visual Studio 2022, CMake, and Win32/x64/ARM64 presets.
- [User guide and limitations](README.TXT): usage, disk accounting, and partial scan results.
- [Backlog and architectural decisions](BACKLOG.md): priorities and distribution/UI constraints.
- [License](LICENSE.txt).

The build explicitly selects C++17. Filesystem enumeration uses wide Windows
APIs; parts of the MFC UI still use ANSI text. Live updates run at most every
250 ms and skip unchanged scans. Failed roots are reported as errors;
unreadable or depth-limited subtrees produce a non-modal partial-result notice.
Partial whole-drive scans also show an “Unavailable (estimate)” treemap block
for the unaccounted volume-used space.

Historical implementation records:

- [Treemap design](docs/superpowers/specs/2026-08-15-treemap-layout-engine-design.md)
- [Treemap plan](docs/superpowers/plans/2026-08-15-treemap-layout-engine.md)
- [Live-scan design](docs/superpowers/specs/2026-08-15-live-progressive-treemap-scan-design.md)
- [Live-scan plan](docs/superpowers/plans/2026-08-15-live-progressive-treemap-scan.md)

These records describe the original work; implementation-status notes identify
subsequent decisions.
