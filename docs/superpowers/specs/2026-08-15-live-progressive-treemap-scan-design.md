# Live Progressive Treemap Rendering During Scan - Design Specification

## Overview

This specification defines the architecture for live progressive treemap rendering in SpaceMonger while background scanning is in progress. As worker threads in `AsyncScanEngine` enumerate directories and files, the main UI thread samples the active folder tree at 10 FPS (every 100 ms) and renders a live, expanding treemap canvas directly behind the classic "Scanning Disk..." progress dialog.

## Goals

1. **Visual Feedback:** Provide dynamic visual animation of the drive space being mapped in real time.
2. **Zero I/O Impact:** Background directory scanning on worker threads runs at 100% NVMe/SSD speed with no disk I/O contention.
3. **Thread Safety & Stability:** Ensure atomic/mutex-protected reading of partial `CFolder` hierarchies without race conditions or memory corruption.
4. **Preserve Classic UI:** Retain the iconic centered "Scanning Disk..." modal dialog with its animated magnifying glass floating cleanly over the live treemap.

---

## Architecture & Components

### 1. `AsyncScanEngine` Live Layout Generation

`AsyncScanEngine` provides a thread-safe snapshotting interface:

```cpp
void AsyncScanEngine::GenerateLiveLayout(
    int w, int h,
    ui64 totalDiskSpace,
    ui64 freeDiskSpace,
    const TreemapConfig& config,
    std::vector<TreemapNode>& outNodes
);
```

#### Synchronization Rules:
- A dedicated `std::mutex m_treeMutex` guards `CFolder` additions (`AddFileWithArena`, `AddFolderWithArena`) and entry array resizes.
- `GenerateLiveLayout` locks `m_treeMutex` briefly to traverse currently attached folders and build a lightweight snapshot.
- Crucially, disk operations (`FindFirstFileW`, `FindNextFileW`, file size queries) execute outside any lock.

### 2. Sizing & Layout Partitioning

During scanning:
- Already scanned files and folders occupy area proportional to their current `SizeTotal()`.
- `<Free Space>` occupies its proportional territory (if `showfreespace` is enabled).
- Undiscovered drive space is represented by a `<Scanning...>` placeholder node whose size equals `max(0, totalDiskSpace - freeDiskSpace - bytesScanned)`.
- As `bytesScanned` increases, the `<Scanning...>` block shrinks until it vanishes upon scan completion.

### 3. UI Message Loop & Throttled Rendering

In `CFolderTree::LoadTree()` (`FolderTree.cpp`):
- The existing loop already pumps messages and updates dialog text every 50 ms.
- A 100 ms timer tick calls `CFolderView::UpdateLiveScanLayout(engine, totalspace, freespace)`.
- `CFolderView` updates its `m_layoutNodes` and invalidates the client area using double-buffered GDI painting.
- `CFolderDialog` remains on top without flicker.

### 4. Scan Completion & Finalization

When `engine.IsScanning()` becomes false:
1. Workers join and finish.
2. `m_rootFolder->Finalize()` sorts the final tree by size descending and computes exact totals.
3. `root` is detached and assigned to `CFolderTree`.
4. `CFolderView` computes the final polished layout with `TreemapEngine::ComputeLayout`.

---

## Verification & Testing Plan

1. **Unit Tests in `tests/AsyncScan_test.cpp`:**
   - Test concurrent `GenerateLiveLayout` calls while multiple worker threads insert 10,000+ files and folders.
   - Test progressive size shrinking of `<Scanning...>` placeholder block.
   - Test cancellation safety while live rendering is active.
2. **Regression Verification:**
   - Full test suite across presets (`vs2022-x64-release`, `vs2022-win32-release`, `vs2022-x64-debug`).
