# Treemap Layout Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the treemap spatial partitioning algorithm and hit-testing out of `CFolderView` into a pure, headless C++ module (`TreemapLayout.h` and `TreemapLayout.cpp`) backed by `std::vector<TreemapNode>`, and add a comprehensive unit test suite `Treemap_test`.

**Architecture:** A pure non-GUI module `TreemapEngine` provides static methods `ComputeLayout`, `GetMinDimensions`, `HitTestItem`, and `HitTestContainer`. It operates on `CFolder` hierarchies, reads a plain `TreemapConfig` struct, and populates `std::vector<TreemapNode>`. `CFolderView` delegates layout calculation and hit-testing to this engine.

**Tech Stack:** C++17, Win32 / MSVC, CMake, CTest.

## Global Constraints

- Standalone pure module: `TreemapLayout.h` and `TreemapLayout.cpp` must NOT include `<afxwin.h>` or depend on MFC `CDC`/`CPoint`/`CRect`.
- Binary compatibility and coordinate conventions: preserve $x, y, w, h$ and margin calculations ($x+3, y+12, w-6, h-15$) exactly.
- Single contiguous storage: replace `CDisplayFolder` linked list with `std::vector<TreemapNode>`.
- All tests must pass on x64 and Win32 presets without memory leaks.

---

### Task 1: Create `TreemapLayout.h` and `TreemapLayout.cpp` Interface

**Files:**
- Create: `TreemapLayout.h`
- Create: `TreemapLayout.cpp`

**Interfaces:**
- Produces:
  - `enum TreemapNodeFlags` (`TREEMAP_FLAG_FOLDER`, `TREEMAP_FLAG_SPECIAL`, `TREEMAP_FLAG_HOVER`)
  - `struct TreemapNode`
  - `struct TreemapConfig`
  - `class TreemapEngine` with declarations for `ComputeLayout`, `GetMinDimensions`, `HitTestItem`, `HitTestContainer`

- [ ] **Step 1: Create `TreemapLayout.h`**

Define `TreemapNode`, `TreemapConfig`, and `TreemapEngine`.

- [ ] **Step 2: Create initial `TreemapLayout.cpp` with `GetMinDimensions`**

Implement `TreemapEngine::GetMinDimensions(int density, int dpi, int &outHMin, int &outVMin)` using `MulDiv` and the standard minimum sizes lookup table.

- [ ] **Step 3: Commit initial files**

```bash
git add TreemapLayout.h TreemapLayout.cpp
git commit -m "feat: introduce TreemapLayout module scaffolding"
```

---

### Task 2: Implement Layout Partitioning and Hit-Testing in `TreemapLayout.cpp`

**Files:**
- Modify: `TreemapLayout.cpp`
- Modify: `TreemapLayout.h`

**Interfaces:**
- Consumes: `CFolder` from `Folder.h`, `TreemapConfig`
- Produces:
  - `TreemapEngine::ComputeLayout(...)`
  - `TreemapEngine::HitTestItem(...)`
  - `TreemapEngine::HitTestContainer(...)`

- [ ] **Step 1: Implement `ComputeLayout` and internal `SizeFolders`**

Extract and adapt the greedy 2-way split, bias aspect-ratio comparison, and recursive subdivision into `TreemapEngine`.

- [ ] **Step 2: Implement `HitTestItem` and `HitTestContainer`**

Implement coordinate hit-testing against `const std::vector<TreemapNode>&` respecting folder border margins and placeholder nodes.

- [ ] **Step 3: Commit implementation**

```bash
git add TreemapLayout.h TreemapLayout.cpp
git commit -m "feat: implement treemap partitioning and hit-testing engine"
```

---

### Task 3: Add `Treemap_test` Unit Test Suite to CMake

**Files:**
- Create: `tests/Treemap_test.cpp`
- Modify: `CMakeLists.txt:140-165`

**Interfaces:**
- Consumes: `TreemapEngine`, `TreemapNode`, `TreemapConfig`, `CFolder`, `CStringArena`
- Produces: Test binary `Treemap_test` registered in CTest

- [ ] **Step 1: Write comprehensive test cases in `tests/Treemap_test.cpp`**

Test scenarios:
1. `NullAndEmptyFolder`: `nullptr` or 0 entries produces empty node vector.
2. `SingleFileLayout`: Single file takes full rectangle bounds.
3. `SingleFolderWithChild`: Produces parent folder node and inner child node with margin bounds ($x+3, y+12, w-6, h-15$).
4. `MinDimensionScaling`: Verifies density $[-3..3]$ and DPI scaling calculations in `GetMinDimensions`.
5. `FreeSpaceToggle`: Validates `<Free Space>` weight handling when `showFreeSpace` is true vs false.
6. `BiasAspectCalculation`: Confirms horizontal vs vertical split orientation depending on positive/negative bias.
7. `DegenerateGeometries`: Zero-byte items, $0 \times 0$ box, $10000 \times 1$ and $1 \times 10000$ aspect ratios.
8. `HitTestingValidation`: Exact hit testing for files, folder headers, inside folder interiors, and container lookup.

- [ ] **Step 2: Add `Treemap_test` target to `CMakeLists.txt`**

Add `Treemap_test` executable with `TreemapLayout.cpp`, `Folder.cpp`, `FolderEntryArrays.cpp`, `PathUtil.cpp`, `DiskUsage.cpp`.

- [ ] **Step 3: Run `ctest --preset vs2022-x64-release` and verify all tests pass**

- [ ] **Step 4: Commit**

```bash
git add tests/Treemap_test.cpp CMakeLists.txt
git commit -m "test: add Treemap_test test suite and register in CMakeLists.txt"
```

---

### Task 4: Integrate `TreemapEngine` into `CFolderView`

**Files:**
- Modify: `FolderView.h:7-16, 77-83, 89-103`
- Modify: `FolderView.cpp:83-96, 227-300, 704-850, 895-1093`
- Modify: `CMakeLists.txt:23-45`

**Interfaces:**
- Consumes: `TreemapEngine`, `TreemapNode`, `TreemapConfig`
- Produces: Updated `CFolderView` using `std::vector<TreemapNode> m_layoutNodes`

- [ ] **Step 1: Update `FolderView.h`**

Replace `CDisplayFolder` linked list with `std::vector<TreemapNode> m_layoutNodes; const TreemapNode *m_selectedNode; const TreemapNode *m_hoverNode;`.
Remove obsolete private methods `BuildFolderLayout`, `SizeFolders`, `AddDisplayFolder`, `ClearDisplayFolders`.

- [ ] **Step 2: Update `FolderView.cpp`**

- Include `TreemapLayout.h`.
- In `OnSize`: construct `TreemapConfig` and call `TreemapEngine::ComputeLayout(...)`.
- In `OnDraw`: iterate over `m_layoutNodes`.
- In `GetDisplayFolderFromPoint` and `GetContainerDisplayFolderFromPoint`: delegate to `TreemapEngine::HitTestItem` and `HitTestContainer`.
- In `MinimalDrawDisplayFolder`, `SelectFolder`, `ZoomIn`, `HighlightPathAtPoint`, tooltips: adapt from `CDisplayFolder*` to `TreemapNode` / `const TreemapNode*`.
- Update `CMakeLists.txt` to include `TreemapLayout.cpp` in `SPACEMONGER_SOURCES`.

- [ ] **Step 3: Build `SpaceMonger` and run all tests**

- [ ] **Step 4: Commit**

```bash
git add FolderView.h FolderView.cpp CMakeLists.txt
git commit -m "refactor: integrate pure TreemapEngine into CFolderView and eliminate raw linked list"
```

---

### Task 5: Final Validation and Backlog Update

**Files:**
- Modify: `BACKLOG.md:24-34`

- [ ] **Step 1: Run full test suite across presets**

Run `ctest --preset vs2022-x64-release` and `ctest --preset vs2022-win32-release` to verify 100% pass rate.

- [ ] **Step 2: Mark task complete in `BACKLOG.md`**

Mark "Extract Treemap Layout Engine into a Pure Module" as completed (`[x]`).

- [ ] **Step 3: Commit**

```bash
git add BACKLOG.md
git commit -m "docs: mark treemap layout engine extraction complete in BACKLOG.md"
```
