# Treemap Layout Engine Pure Module Extraction Design

**Author:** Antigravity  
**Date:** 2026-08-15  
**Status:** Approved  
**Target:** SpaceMonger v1.4.x  

## 1. Overview and Goals

SpaceMonger's treemap visualization relies on a recursive spatial partitioning algorithm that divides rectangles proportionally based on file and folder sizes. Currently, this partitioning logic (`BuildFolderLayout`, `SizeFolders`) and the resulting display list (`CDisplayFolder` linked list) are implemented directly inside `CFolderView`, interleaved with MFC window messages, device context rendering (`CDC`), and application settings.

### Goals:
1. **Decouple Partitioning Logic:** Extract treemap partitioning and hit-testing into a pure, headless C++ module (`TreemapLayout.h`, `TreemapLayout.cpp`) that has zero dependencies on MFC or Win32 GUI APIs.
2. **Modernize Node Storage:** Replace the 1990s raw heap-allocated linked list (`CDisplayFolder`) with a contiguous, cache-friendly `std::vector<TreemapNode>`.
3. **Comprehensive Headless Unit Testing:** Create `Treemap_test.cpp` in `tests/` and add it to `CMakeLists.txt` to enable automated unit testing of layout coordinates, aspect-ratio bias, density/DPI scaling, degenerate geometries, and hit-testing without needing an initialized Win32 window.

---

## 2. Architecture & Data Structures

### 2.1 Node Flags & Structs (`TreemapLayout.h`)

```cpp
#pragma once

#include <vector>
#include <string>
#include "e.h"
#include "Folder.h"

enum TreemapNodeFlags : ui32 {
    TREEMAP_FLAG_FOLDER   = (1 << 0), // 1: Node represents a directory / folder
    TREEMAP_FLAG_SPECIAL  = (1 << 1), // 2: Special/Free space block (<Free Space>, etc.)
    TREEMAP_FLAG_HOVER    = (1 << 2), // 4: Mouse hover highlight
};

struct TreemapNode {
    const wchar_t *name{nullptr};
    si32 depth{0};
    ui32 flags{0};
    CFolder *source{nullptr};
    ui32 index{static_cast<ui32>(-1)};
    si16 x{0}, y{0}, w{0}, h{0};

    bool IsFolder() const { return (flags & TREEMAP_FLAG_FOLDER) != 0; }
    bool IsSpecial() const { return (flags & TREEMAP_FLAG_SPECIAL) != 0; }
    bool IsHovered() const { return (flags & TREEMAP_FLAG_HOVER) != 0; }
};

struct TreemapConfig {
    int density{0};       // Range [-3..3], where 0 is default
    int bias{0};          // Aspect ratio preference [-8..8]
    bool showFreeSpace{true};
    int dpi{96};          // Target DPI for minimum sizing thresholds
};
```

---

## 3. Algorithm & Partitioning Details

### 3.1 Sizing Thresholds (`GetMinDimensions`)

Minimum partition dimensions are computed using the density setting index (`density + 3`) and scaled by DPI:

| Density Level | Base Dimensions (w x h) |
|---|---|
| -3 | 96 x 64 |
| -2 | 64 x 48 |
| -1 | 48 x 32 |
|  0 (Default) | 32 x 24 |
| +1 | 24 x 16 |
| +2 | 16 x 12 |
| +3 | 8 x 6 |

$$\text{hmin} = \text{MulDiv}(\text{minsizes}[\text{density} + 3][0], \text{dpi}, 96)$$
$$\text{vmin} = \text{MulDiv}(\text{minsizes}[\text{density} + 3][1], \text{dpi}, 96)$$

### 3.2 Greedy 2-Way Partitioning

Given a parent rectangle $(x, y, w, h)$ and folder entry indices sorted by size descending:
1. Filter entries with zero effective size (including `<Free Space>` when `showFreeSpace == false`).
2. Iteratively assign entries to `list1` and `list2` to minimize $| \sum \text{size}(\text{list1}) - \sum \text{size}(\text{list2}) |$.
3. Compute aspect ratio with bias:
   $$\text{wbias} = (\text{bias} > 0) ? (\text{bias} + 8) : 8$$
   $$\text{hbias} = (\text{bias} < 0) ? (-\text{bias} + 8) : 8$$
   If $(w \cdot \text{wbias}) > (h \cdot \text{hbias})$, split horizontally (along width); otherwise, split vertically (along height).
4. Subdivide partitions recursively if $w_i > \text{hmin}$ and $h_i > \text{vmin}$.
5. For leaf folder nodes large enough to show nested contents, recurse into `folder->children[i]` within inner bounds $(x + 3, y + 12, w - 6, h - 15)$.

### 3.3 Hit-Testing Engine

1. **`HitTestItem(nodes, px, py)`:**
   - Linear scan finding the deepest matching node.
   - For folders (`flags & TREEMAP_FLAG_FOLDER`), hits are valid only in the header or outer border band ($px < x+3 \lor py < y+12 \lor px > x+w-3 \lor py > y+h-3$), ensuring child files within the interior are selectable.
   - Ignores null or unnamed placeholder nodes.
2. **`HitTestContainer(nodes, px, py)`:**
   - Finds the smallest enclosing folder rectangle $(x, y, w, h)$ that contains $(px, py)$.

---

## 4. `CFolderView` Modernization

1. **Member Variables:**
   - Replace `CDisplayFolder *displayfolders, *displayend;` with `std::vector<TreemapNode> m_layoutNodes;`.
   - Replace `CDisplayFolder *selected;` with `const TreemapNode *m_selectedNode{nullptr};`.
   - Replace `CDisplayFolder *lastcur;` with `const TreemapNode *m_hoverNode{nullptr};`.
2. **Lifecycle & Rendering:**
   - In `OnSize`: construct `TreemapConfig` and call `TreemapEngine::ComputeLayout(0, 0, cx - 1, cy - 1, rootfolder, zoomlevel, config, m_layoutNodes);`.
   - In `OnDraw`: iterate directly over `m_layoutNodes` with `for (const auto &node : m_layoutNodes)`.
   - Remove manual `delete` linked-list traversal in `ClearDisplayFolders` / `OnDestroy`.

---

## 5. Automated Verification Plan

### Test Suite (`tests/Treemap_test.cpp`)

1. `NullAndEmptyFolder`: Verifies handling of `nullptr` and 0-entry folders without crashes or allocations.
2. `SingleFileAndFolderLayout`: Verifies single file fills entire rect; single folder adds folder box and inner child box.
3. `AspectSplitAndBias`: Verifies splitting orientation on wide vs tall rectangles and bias influence.
4. `MinDimensionDpiScaling`: Tests `GetMinDimensions` across all densities `[-3..3]` and high DPI values (96, 144, 192).
5. `FreeSpaceVisibility`: Verifies `<Free Space>` item inclusion/exclusion based on `showFreeSpace`.
6. `DegenerateGeometries`: Tests $0 \times 0$, $10000 \times 1$, $1 \times 10000$, and zero-byte sizes.
7. `HitTesting`: Tests item hit testing in folder header vs interior files, and container folder resolution.
