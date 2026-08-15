# Live Progressive Treemap Rendering During Scan Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement live progressive treemap rendering during async disk scanning in SpaceMonger, allowing folders to dynamically populate the canvas behind the scanning dialog.

**Architecture:** `AsyncScanEngine` exposes a thread-safe `GenerateLiveLayout` snapshot method guarded by a lightweight mutex around folder tree modifications. The main UI loop in `CFolderTree::LoadTree` samples layout snapshots at 10 FPS (100 ms interval) and invalidates `CFolderView` to render the growing treemap behind `CFolderDialog`.

**Tech Stack:** C++17, Win32 / MFC, CMake, CTest.

**Spec:** `docs/superpowers/specs/2026-08-15-live-progressive-treemap-scan-design.md`

## Global Constraints

- No disk I/O performance degradation on worker threads.
- Thread-safe tree snapshotting without data races on `CFolder` structures.
- Classic `CFolderDialog` remains on top without flicker.
- All automated unit tests pass on x64, Win32, and debug configurations.

---

### Task 1: Add Thread Synchronization and `GenerateLiveLayout` to `AsyncScanEngine`

**Files:**
- Modify: `AsyncScanEngine.h:20-98`
- Modify: `AsyncScanEngine.cpp:50-350`
- Test: `tests/AsyncScan_test.cpp:1-180`

**Interfaces:**
- Consumes: `CFolder`, `TreemapEngine`, `TreemapNode`, `TreemapConfig`
- Produces:
  ```cpp
  void AsyncScanEngine::GenerateLiveLayout(
      int w, int h,
      ui64 totalDiskSpace,
      ui64 freeDiskSpace,
      const TreemapConfig& config,
      std::vector<TreemapNode>& outNodes
  );
  ```

- [ ] **Step 1: Update `AsyncScanEngine.h` to declare `m_treeMutex` and `GenerateLiveLayout`**

Add `mutable std::mutex m_treeMutex;` and `void GenerateLiveLayout(...)`.

- [ ] **Step 2: Implement tree synchronization and `GenerateLiveLayout` in `AsyncScanEngine.cpp`**

Guard `CFolder::AddFileWithArena` and `CFolder::AddFolderWithArena` calls in `AsyncScanEngine.cpp` with `std::lock_guard<std::mutex> lock(m_treeMutex);`.
Implement `GenerateLiveLayout` to snapshot currently attached folders, append `<Free Space>` and `<Scanning...>` placeholder nodes, and call `TreemapEngine::ComputeLayout`.

- [ ] **Step 3: Write multi-threaded unit tests in `tests/AsyncScan_test.cpp`**

Add `test_concurrent_live_layout()` verifying rapid concurrent calls to `GenerateLiveLayout` while worker threads scan directories.

- [ ] **Step 4: Build and run test suite**

Run `cmake --build --preset vs2022-x64-release` and `ctest --preset vs2022-x64-release -R AsyncScan_test`.

- [ ] **Step 5: Commit**

```bash
git add AsyncScanEngine.h AsyncScanEngine.cpp tests/AsyncScan_test.cpp
git commit -m "feat: implement thread-safe live layout generation in AsyncScanEngine"
```

---

### Task 2: Implement UI Hook in `CFolderView` and Throttled Scan Loop in `CFolderTree`

**Files:**
- Modify: `FolderView.h:80-110`
- Modify: `FolderView.cpp:200-300`
- Modify: `FolderTree.cpp:60-120`

**Interfaces:**
- Consumes: `AsyncScanEngine::GenerateLiveLayout`
- Produces: `void CFolderView::UpdateLiveScanLayout(AsyncScanEngine& engine, ui64 totalspace, ui64 freespace);`

- [ ] **Step 1: Declare `UpdateLiveScanLayout` in `FolderView.h`**

Add public method `void UpdateLiveScanLayout(AsyncScanEngine& engine, ui64 totalspace, ui64 freespace);`.

- [ ] **Step 2: Implement `UpdateLiveScanLayout` in `FolderView.cpp`**

Retrieve client area dimensions, call `engine.GenerateLiveLayout`, and request an immediate double-buffered redraw (`Invalidate(FALSE); UpdateWindow();`).

- [ ] **Step 3: Add 100ms render tick to `CFolderTree::LoadTree()` in `FolderTree.cpp`**

During the active scan loop, trigger `fv->UpdateLiveScanLayout(engine, totalspace, freespace)` every 100 ms.

- [ ] **Step 4: Build SpaceMonger executable and verify compilation**

Run `cmake --build --preset vs2022-x64-release`.

- [ ] **Step 5: Commit**

```bash
git add FolderView.h FolderView.cpp FolderTree.cpp
git commit -m "feat: hook progressive live treemap updates into CFolderView and CFolderTree"
```

---

### Task 3: Regression Verification and Backlog Documentation

**Files:**
- Modify: `BACKLOG.md`

- [ ] **Step 1: Run full test suite across all presets**

Run `ctest --preset vs2022-x64-release`, `ctest --preset vs2022-win32-release`, and `ctest --preset vs2022-x64-debug`.

- [ ] **Step 2: Update `BACKLOG.md`**

Add record of live progressive scanning feature.

- [ ] **Step 3: Commit**

```bash
git add BACKLOG.md
git commit -m "docs: update backlog with progressive live treemap scanning feature"
```
