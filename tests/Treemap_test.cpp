#include "../TreemapLayout.h"
#include "../Folder.h"
#include "../StringArena.h"

#include <stdio.h>
#include <vector>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			fflush(stderr); \
			return 0; \
		} \
	} while (0)

static int test_null_and_empty_folder()
{
	std::vector<TreemapNode> nodes;
	TreemapConfig config;

	// Null root
	TreemapEngine::ComputeLayout(0, 0, 800, 600, nullptr, 0, config, nodes);
	CHECK(nodes.empty());

	// Hit-testing on empty
	CHECK(TreemapEngine::HitTestItem(nodes, 100, 100) == nullptr);
	CHECK(TreemapEngine::HitTestContainer(nodes, 100, 100) == nullptr);

	// Empty folder (cur = 0)
	CStringArena arena;
	CFolder emptyFolder;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &emptyFolder, 0, config, nodes);
	CHECK(nodes.empty());

	return 1;
}

static int test_single_file_partitioning()
{
	CStringArena arena;
	CFolder folder;
	folder.AddFileWithArena(arena, L"file1.txt", 9, 1000, 1000, 0);
	folder.Finalize();

	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &folder, 0, config, nodes);

	CHECK(nodes.size() == 1);
	CHECK(nodes[0].name != nullptr);
	CHECK(wcscmp(nodes[0].name, L"file1.txt") == 0);
	CHECK(!nodes[0].IsFolder());
	CHECK(!nodes[0].IsSpecial());
	CHECK(nodes[0].x == 0 && nodes[0].y == 0);
	CHECK(nodes[0].w == 800 && nodes[0].h == 600);
	CHECK(nodes[0].depth == 0);

	return 1;
}

static int test_single_folder_with_children()
{
	CStringArena arena;
	CFolder *subFolder = new CFolder;
	subFolder->AddFileWithArena(arena, L"inner.txt", 9, 500, 500, 0);

	CFolder root;
	root.AddFolderWithArena(arena, L"SubDir", 6, subFolder, 0);
	root.Finalize();

	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &root, 0, config, nodes);

	// Expect 2 nodes: SubDir folder node, then inner.txt child node
	CHECK(nodes.size() == 2);
	CHECK(nodes[0].IsFolder());
	CHECK(wcscmp(nodes[0].name, L"SubDir") == 0);
	CHECK(nodes[0].x == 0 && nodes[0].y == 0);
	CHECK(nodes[0].w == 800 && nodes[0].h == 600);
	CHECK(nodes[0].depth == 0);

	// Inner file should have margin offset (x+3, y+12, w-6, h-15)
	CHECK(!nodes[1].IsFolder());
	CHECK(wcscmp(nodes[1].name, L"inner.txt") == 0);
	CHECK(nodes[1].x == 3 && nodes[1].y == 12);
	CHECK(nodes[1].w == 800 - 6 && nodes[1].h == 600 - 15);
	CHECK(nodes[1].depth == 1);

	return 1;
}

static int test_min_dimension_dpi_scaling()
{
	int hmin = 0, vmin = 0;

	// Default density 0 at 96 DPI: { 32, 24 }
	TreemapEngine::GetMinDimensions(0, 96, hmin, vmin);
	CHECK(hmin == 32);
	CHECK(vmin == 24);

	// Density -3 (largest min boxes: {96, 64})
	TreemapEngine::GetMinDimensions(-3, 96, hmin, vmin);
	CHECK(hmin == 96);
	CHECK(vmin == 64);

	// Density +3 (smallest min boxes: {8, 6})
	TreemapEngine::GetMinDimensions(3, 96, hmin, vmin);
	CHECK(hmin == 8);
	CHECK(vmin == 6);

	// High DPI (192 DPI = 200% scaling) at density 0: { 64, 48 }
	TreemapEngine::GetMinDimensions(0, 192, hmin, vmin);
	CHECK(hmin == 64);
	CHECK(vmin == 48);

	// Clamp handling for out-of-range density
	TreemapEngine::GetMinDimensions(-10, 96, hmin, vmin);
	CHECK(hmin == 96); // Clamped to -3
	TreemapEngine::GetMinDimensions(10, 96, hmin, vmin);
	CHECK(hmin == 8);  // Clamped to +3

	return 1;
}

static int test_aspect_split_and_bias()
{
	CStringArena arena;
	CFolder folder;
	folder.AddFileWithArena(arena, L"fileA.txt", 10, 1000, 1000, 0);
	folder.AddFileWithArena(arena, L"fileB.txt", 10, 1000, 1000, 0);
	folder.Finalize();

	// Wide rectangle (800x200), default bias: splits horizontally (along width)
	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 800, 200, &folder, 0, config, nodes);

	CHECK(nodes.size() == 2);
	CHECK(nodes[0].w == 400 && nodes[0].h == 200);
	CHECK(nodes[1].w == 400 && nodes[1].h == 200);
	CHECK(nodes[0].x == 0 && nodes[1].x == 400);

	// Tall rectangle (200x800), default bias: splits vertically (along height)
	nodes.clear();
	TreemapEngine::ComputeLayout(0, 0, 200, 800, &folder, 0, config, nodes);
	CHECK(nodes.size() == 2);
	CHECK(nodes[0].w == 200 && nodes[0].h == 400);
	CHECK(nodes[1].w == 200 && nodes[1].h == 400);
	CHECK(nodes[0].y == 0 && nodes[1].y == 400);

	return 1;
}

static int test_free_space_visibility()
{
	CStringArena arena;
	CFolder folder;
	folder.AddFileWithArena(arena, L"data.bin", 8, 1000, 1000, 0);
	folder.AddFileWithArena(arena, L"<Free Space>", 12, 1000, 1000, 0);
	folder.Finalize();

	// showFreeSpace = true: both items partitioned
	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	config.showFreeSpace = true;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &folder, 0, config, nodes);

	CHECK(nodes.size() == 2);
	bool foundFree = false;
	for (const auto &n : nodes) {
		if (n.name != nullptr && wcscmp(n.name, L"<Free Space>") == 0) {
			foundFree = true;
			CHECK(n.IsSpecial());
		}
	}
	CHECK(foundFree);

	// showFreeSpace = false: <Free Space> has 0 effective size, data.bin occupies full area
	nodes.clear();
	config.showFreeSpace = false;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &folder, 0, config, nodes);

	CHECK(nodes.size() == 1);
	CHECK(wcscmp(nodes[0].name, L"data.bin") == 0);
	CHECK(nodes[0].w == 800 && nodes[0].h == 600);

	return 1;
}

static int test_degenerate_geometries()
{
	CStringArena arena;
	CFolder folder;
	folder.AddFileWithArena(arena, L"zero.txt", 8, 0, 0, 0);
	folder.Finalize();

	TreemapConfig config;
	std::vector<TreemapNode> nodes;

	// Zero-byte total size
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &folder, 0, config, nodes);
	CHECK(nodes.empty());

	// 0x0 viewport
	CFolder normalFolder;
	normalFolder.AddFileWithArena(arena, L"file.txt", 8, 100, 100, 0);
	normalFolder.Finalize();
	nodes.clear();
	TreemapEngine::ComputeLayout(0, 0, 0, 0, &normalFolder, 0, config, nodes);
	CHECK(nodes.empty());

	// Extreme aspect ratios: 10000x1 and 1x10000
	nodes.clear();
	TreemapEngine::ComputeLayout(0, 0, 10000, 1, &normalFolder, 0, config, nodes);
	// Does not crash, min dimensions filter small splits safely
	nodes.clear();
	TreemapEngine::ComputeLayout(0, 0, 1, 10000, &normalFolder, 0, config, nodes);
	// Does not crash

	return 1;
}

static int test_hit_testing_leaf_and_container()
{
	CStringArena arena;
	CFolder *subFolder = new CFolder;
	subFolder->AddFileWithArena(arena, L"file1.txt", 9, 500, 500, 0);

	CFolder root;
	root.AddFolderWithArena(arena, L"FolderA", 7, subFolder, 0);
	root.Finalize();

	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 400, 300, &root, 0, config, nodes);

	CHECK(nodes.size() == 2);
	// Node 0: FolderA at (0, 0, 400, 300)
	// Node 1: file1.txt at (3, 12, 394, 285)

	// Hit test in header of FolderA (e.g. at (20, 5))
	const TreemapNode *hit = TreemapEngine::HitTestItem(nodes, 20, 5);
	CHECK(hit != nullptr);
	CHECK(hit->IsFolder());
	CHECK(wcscmp(hit->name, L"FolderA") == 0);

	// Hit test in child file interior (e.g. at (50, 50))
	hit = TreemapEngine::HitTestItem(nodes, 50, 50);
	CHECK(hit != nullptr);
	CHECK(!hit->IsFolder());
	CHECK(wcscmp(hit->name, L"file1.txt") == 0);

	// Hit test container at (50, 50) -> should be FolderA
	const TreemapNode *container = TreemapEngine::HitTestContainer(nodes, 50, 50);
	CHECK(container != nullptr);
	CHECK(container->IsFolder());
	CHECK(wcscmp(container->name, L"FolderA") == 0);

	// Hit test outside layout bounds
	CHECK(TreemapEngine::HitTestItem(nodes, 500, 500) == nullptr);
	CHECK(TreemapEngine::HitTestContainer(nodes, 500, 500) == nullptr);

	return 1;
}

static int test_multi_level_nesting()
{
	CStringArena arena;
	CFolder *level2 = new CFolder;
	level2->AddFileWithArena(arena, L"deep.dat", 8, 200, 200, 0);

	CFolder *level1 = new CFolder;
	level1->AddFolderWithArena(arena, L"Level2", 6, level2, 0);

	CFolder root;
	root.AddFolderWithArena(arena, L"Level1", 6, level1, 0);
	root.Finalize();

	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 800, 600, &root, 0, config, nodes);

	// Expect 3 nodes: Level1 (depth 0), Level2 (depth 1), deep.dat (depth 2)
	CHECK(nodes.size() == 3);
	CHECK(nodes[0].depth == 0 && nodes[0].IsFolder() && wcscmp(nodes[0].name, L"Level1") == 0);
	CHECK(nodes[1].depth == 1 && nodes[1].IsFolder() && wcscmp(nodes[1].name, L"Level2") == 0);
	CHECK(nodes[2].depth == 2 && !nodes[2].IsFolder() && wcscmp(nodes[2].name, L"deep.dat") == 0);

	// Check cumulative margin nesting:
	// Level 1: (0, 0, 800, 600)
	// Level 2: (3, 12, 794, 585)
	// deep.dat: (3+3, 12+12, 794-6, 585-15) = (6, 24, 788, 570)
	CHECK(nodes[1].x == 3 && nodes[1].y == 12 && nodes[1].w == 794 && nodes[1].h == 585);
	CHECK(nodes[2].x == 6 && nodes[2].y == 24 && nodes[2].w == 788 && nodes[2].h == 570);

	return 1;
}

static int test_multi_file_greedy_partition()
{
	CStringArena arena;
	CFolder root;
	// 400 + 100 = 500; 300 + 200 = 500 (perfect balance)
	root.AddFileWithArena(arena, L"f400.txt", 8, 400, 400, 0);
	root.AddFileWithArena(arena, L"f300.txt", 8, 300, 300, 0);
	root.AddFileWithArena(arena, L"f200.txt", 8, 200, 200, 0);
	root.AddFileWithArena(arena, L"f100.txt", 8, 100, 100, 0);
	root.Finalize();

	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 800, 400, &root, 0, config, nodes);

	CHECK(nodes.size() == 4);
	for (const auto &n : nodes) {
		CHECK(n.w > 0 && n.h > 0);
		CHECK(n.x >= 0 && n.y >= 0);
		CHECK(n.x + n.w <= 800);
		CHECK(n.y + n.h <= 400);
	}

	return 1;
}

static int test_extreme_bias_and_density_filtering()
{
	CStringArena arena;
	CFolder root;
	root.AddFileWithArena(arena, L"f1.txt", 6, 500, 500, 0);
	root.AddFileWithArena(arena, L"f2.txt", 6, 500, 500, 0);
	root.Finalize();

	// Extreme positive bias (+8) on a square box (400x400) forces horizontal split (splits width)
	TreemapConfig config;
	config.bias = 8;
	std::vector<TreemapNode> nodes;
	TreemapEngine::ComputeLayout(0, 0, 400, 400, &root, 0, config, nodes);
	CHECK(nodes.size() == 2);
	CHECK(nodes[0].w == 200 && nodes[0].h == 400);
	CHECK(nodes[1].w == 200 && nodes[1].h == 400);

	// Extreme negative bias (-8) on a square box (400x400) forces vertical split (splits height)
	config.bias = -8;
	nodes.clear();
	TreemapEngine::ComputeLayout(0, 0, 400, 400, &root, 0, config, nodes);
	CHECK(nodes.size() == 2);
	CHECK(nodes[0].w == 400 && nodes[0].h == 200);
	CHECK(nodes[1].w == 400 && nodes[1].h == 200);

	return 1;
}

// Regression test: sub-threshold regions must emit visible, correctly placed
// placeholder blocks. A placeholder once rendered invisibly (depth -1 draws
// flat gray with no outline), which left dense folders looking empty, and an
// earlier copy-paste bug placed list1's placeholder in list2's rectangle.
static int test_subthreshold_placeholder_depth_and_bounds()
{
	CStringArena arena;
	CFolder root;
	wchar_t name[16];
	for (int i = 0; i < 32; i++) {
		swprintf(name, 16, L"f%02d.bin", i);
		root.AddFileWithArena(arena, name, (ui32)wcslen(name), 10, 10, 0);
	}
	root.Finalize();

	// 40x30 exceeds the default minimum box (32x24 at 96 DPI), but each half
	// of the first split (20x30) falls below it, so both halves must become
	// placeholder blocks.
	std::vector<TreemapNode> nodes;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 40, 30, &root, 0, config, nodes);

	CHECK(nodes.size() == 2);
	for (const auto &n : nodes) {
		// Placeholder identity: no name, no source index.
		CHECK(n.name == nullptr);
		CHECK(n.index == (ui32)-1);
		// Must carry the parent depth so it renders as a depth-colored
		// block, never -1 (which renders as invisible flat gray).
		CHECK(n.depth == 0);
		// Must lie within the region being subdivided.
		CHECK(n.x >= 0 && n.y >= 0);
		CHECK(n.x + n.w <= 40);
		CHECK(n.y + n.h <= 30);
		CHECK(n.w > 0 && n.h > 0);
	}
	// The two placeholders must tile the region, not overlap.
	CHECK(nodes[0].x + nodes[0].w <= nodes[1].x || nodes[1].x + nodes[1].w <= nodes[0].x
		|| nodes[0].y + nodes[0].h <= nodes[1].y || nodes[1].y + nodes[1].h <= nodes[0].y);

	return 1;
}

// Regression test: the layout must not depend on the order entries were
// added. Live-scan trees arrive unsorted (insertion order), while finalized
// trees are sorted by size; the engine sorts internally so both must produce
// identical geometry for identical content.
static int test_unsorted_input_matches_finalized_layout()
{
	static const ui64 SIZES[] = { 800, 700, 600, 500, 400, 300, 200, 100 };
	static const int COUNT = 8;
	wchar_t name[16];

	CStringArena arenaA;
	CFolder sorted;
	for (int i = 0; i < COUNT; i++) {
		swprintf(name, 16, L"s%llu.bin", SIZES[i]);
		sorted.AddFileWithArena(arenaA, name, (ui32)wcslen(name), SIZES[i], SIZES[i], 0);
	}
	sorted.Finalize();

	CStringArena arenaB;
	CFolder unsorted;
	for (int i = COUNT - 1; i >= 0; i--) {
		swprintf(name, 16, L"s%llu.bin", SIZES[i]);
		unsorted.AddFileWithArena(arenaB, name, (ui32)wcslen(name), SIZES[i], SIZES[i], 0);
	}
	// Deliberately NOT finalized: entries stay in ascending insertion order,
	// like a live scan tree.

	std::vector<TreemapNode> nodesSorted, nodesUnsorted;
	TreemapConfig config;
	TreemapEngine::ComputeLayout(0, 0, 1200, 900, &sorted, 0, config, nodesSorted);
	TreemapEngine::ComputeLayout(0, 0, 1200, 900, &unsorted, 0, config, nodesUnsorted);

	CHECK(nodesSorted.size() == (size_t)COUNT);
	CHECK(nodesUnsorted.size() == (size_t)COUNT);

	// Every file must land at the same rectangle in both layouts.
	for (const auto &a : nodesSorted) {
		CHECK(a.name != nullptr);
		bool found = false;
		for (const auto &b : nodesUnsorted) {
			if (b.name != nullptr && wcscmp(a.name, b.name) == 0) {
				CHECK(a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h);
				CHECK(a.depth == b.depth);
				found = true;
				break;
			}
		}
		CHECK(found);
	}

	return 1;
}

int main()
{
	if (!test_null_and_empty_folder()) return 1;
	if (!test_single_file_partitioning()) return 1;
	if (!test_single_folder_with_children()) return 1;
	if (!test_min_dimension_dpi_scaling()) return 1;
	if (!test_aspect_split_and_bias()) return 1;
	if (!test_free_space_visibility()) return 1;
	if (!test_degenerate_geometries()) return 1;
	if (!test_hit_testing_leaf_and_container()) return 1;
	if (!test_multi_level_nesting()) return 1;
	if (!test_multi_file_greedy_partition()) return 1;
	if (!test_extreme_bias_and_density_filtering()) return 1;
	if (!test_subthreshold_placeholder_depth_and_bounds()) return 1;
	if (!test_unsorted_input_matches_finalized_layout()) return 1;

	printf("Treemap_test passed successfully.\n");
	return 0;
}

