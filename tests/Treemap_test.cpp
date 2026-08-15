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

	printf("Treemap_test passed successfully.\n");
	return 0;
}

