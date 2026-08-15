#include "TreemapLayout.h"
#include <windows.h>

void TreemapEngine::GetMinDimensions(int density, int dpi, int &outHMin, int &outVMin)
{
	static const int minsizes[][2] = {
		{ 96, 64, },
		{ 64, 48, },
		{ 48, 32, },
		{ 32, 24, },
		{ 24, 16, },
		{ 16, 12, },
		{ 8,  6,  },
	};

	int d = density + 3;
	if (d < 0) d = 0;
	if (d > 6) d = 6;
	if (dpi <= 0) dpi = 96;

	outHMin = MulDiv(minsizes[d][0], dpi, 96);
	outVMin = MulDiv(minsizes[d][1], dpi, 96);
}

static void AddNode(
	std::vector<TreemapNode> &nodes,
	CFolder *source, ui32 index,
	si32 depth, si16 x, si16 y, si16 w, si16 h, ui32 flags)
{
	TreemapNode node;
	wchar_t c = 0;

	if (source != nullptr && index != (ui32)-1) {
		node.name = source->names[index];
		if (node.name != nullptr) c = node.name[0];
	} else {
		node.name = nullptr;
	}

	if (c == L'*' || c == L'<' || c == L'>' || c == L'?' || c == L'|') {
		node.depth = -1;
		flags |= TREEMAP_FLAG_SPECIAL;
	} else {
		node.depth = depth;
	}

	node.flags = flags;
	node.source = source;
	node.index = index;
	node.x = x;
	node.y = y;
	node.w = w;
	node.h = h;

	nodes.push_back(node);
}

static void ComputeLayoutInternal(
	int x, int y, int w, int h,
	CFolder *folder,
	int depth,
	const TreemapConfig &config,
	int hmin, int vmin,
	std::vector<TreemapNode> &outNodes);

static void SizeFolders(
	int x, int y, int w, int h,
	CFolder *folder,
	int *index, int *scratch, int numindices, int depth,
	const TreemapConfig &config,
	int hmin, int vmin,
	std::vector<TreemapNode> &outNodes)
{
	if (folder == nullptr || numindices <= 0) return;

	int numlist1 = 0, numlist2 = 0, largest;
	int list2_back = numindices;
	si64 list1sum = 0, list2sum = 0, bignum;
	int x1, y1, w1, h1;
	int x2, y2, w2, h2;
	int split;

	// Split the lists evenly. We assume the sizes are sorted
	// in descending order. Overall, this is a greedy algorithm,
	// so it should produce fairly good results.
	for (largest = 0; largest < numindices; largest++) {
		bignum = (si64)folder->sizes[index[largest]];
		if (folder->names[index[largest]] != nullptr
			&& folder->names[index[largest]][0] == L'<'
			&& !config.showFreeSpace) {
			bignum = 0;
		}
		if (bignum != 0) {
			if (list1sum <= list2sum) {
				scratch[numlist1++] = index[largest];
				list1sum += bignum;
			} else {
				scratch[--list2_back] = index[largest];
				list2sum += bignum;
				numlist2++;
			}
		}
	}

	// Don't bother if the files have no space
	if (list1sum + list2sum <= 0) {
		return;
	}

	// Copy lists back into the index array
	for (int i = 0; i < numlist1; i++) {
		index[i] = scratch[i];
	}
	for (int i = 0; i < numlist2; i++) {
		index[numlist1 + i] = scratch[numindices - 1 - i];
	}

	// Calculate aspect ratio with bias to determine split axis
	int wbias = 8, hbias = 8;
	if (config.bias > 0) {
		wbias = config.bias + 8;
	} else if (config.bias < 0) {
		hbias = -config.bias + 8;
	}

	if (((w * wbias) / 8) > ((h * hbias) / 8)) {
		split = (int)(((si64)w * list1sum) / (list1sum + list2sum));
		x1 = x; y1 = y; w1 = split; h1 = h;
		x2 = x + split; y2 = y; w2 = w - split; h2 = h;
	} else {
		split = (int)(((si64)h * list1sum) / (list1sum + list2sum));
		x1 = x; y1 = y; w1 = w; h1 = split;
		x2 = x; y2 = y + split; w2 = w; h2 = h - split;
	}

	// Subdivide list1
	if (numlist1 > 1 && w1 > hmin && h1 > vmin) {
		SizeFolders(x1, y1, w1, h1, folder, index, scratch, numlist1, depth, config, hmin, vmin, outNodes);
	} else if (numlist1 > 0) {
		if (w1 > hmin && h1 > vmin) {
			AddNode(outNodes, folder, index[0],
				depth, (si16)x1, (si16)y1, (si16)w1, (si16)h1,
				(folder->children[index[0]] != nullptr ? TREEMAP_FLAG_FOLDER : TREEMAP_FLAG_NONE));
			if (folder->children[index[0]] != nullptr) {
				if (w1 > hmin && h1 > vmin) {
					ComputeLayoutInternal(x1 + 3, y1 + 12, w1 - 6, h1 - 15,
						folder->children[index[0]], depth + 1, config, hmin, vmin, outNodes);
				} else {
					AddNode(outNodes, folder, (ui32)-1, depth + 1,
						(si16)(x2 + 3), (si16)(y2 + 12), (si16)(w2 - 6), (si16)(h2 - 15), TREEMAP_FLAG_NONE);
				}
			}
		} else {
			AddNode(outNodes, folder, (ui32)-1, depth, (si16)x1, (si16)y1, (si16)w1, (si16)h1, TREEMAP_FLAG_NONE);
		}
	}

	// Subdivide list2
	if (numlist2 > 1 && w2 > hmin && h2 > vmin) {
		SizeFolders(x2, y2, w2, h2, folder, index + numlist1, scratch, numlist2, depth, config, hmin, vmin, outNodes);
	} else if (numlist2 > 0) {
		if (w2 > hmin && h2 > vmin) {
			AddNode(outNodes, folder, index[numlist1],
				depth, (si16)x2, (si16)y2, (si16)w2, (si16)h2,
				(folder->children[index[numlist1]] != nullptr ? TREEMAP_FLAG_FOLDER : TREEMAP_FLAG_NONE));
			if (folder->children[index[numlist1]] != nullptr) {
				if (w2 > hmin && h2 > vmin) {
					ComputeLayoutInternal(x2 + 3, y2 + 12, w2 - 6, h2 - 15,
						folder->children[index[numlist1]], depth + 1, config, hmin, vmin, outNodes);
				} else {
					AddNode(outNodes, folder, (ui32)-1, depth + 1,
						(si16)(x2 + 3), (si16)(y2 + 12), (si16)(w2 - 6), (si16)(h2 - 15), TREEMAP_FLAG_NONE);
				}
			}
		} else {
			AddNode(outNodes, folder, (ui32)-1, depth, (si16)x2, (si16)y2, (si16)w2, (si16)h2, TREEMAP_FLAG_NONE);
		}
	}
}

static void ComputeLayoutInternal(
	int x, int y, int w, int h,
	CFolder *folder,
	int depth,
	const TreemapConfig &config,
	int hmin, int vmin,
	std::vector<TreemapNode> &outNodes)
{
	if (folder == nullptr || folder->cur == 0 || w <= 0 || h <= 0) return;

	std::vector<int> indices(folder->cur);
	for (unsigned int i = 0; i < folder->cur; i++) indices[i] = (int)i;

	std::vector<int> scratch(folder->cur);
	SizeFolders(x, y, w, h, folder, indices.data(), scratch.data(), (int)folder->cur, depth, config, hmin, vmin, outNodes);
}

void TreemapEngine::ComputeLayout(
	int x, int y, int w, int h,
	CFolder *root,
	int startDepth,
	const TreemapConfig &config,
	std::vector<TreemapNode> &outNodes)
{
	outNodes.clear();
	if (root == nullptr || root->cur == 0 || w <= 0 || h <= 0) return;

	int hmin = 0, vmin = 0;
	GetMinDimensions(config.density, config.dpi, hmin, vmin);

	ComputeLayoutInternal(x, y, w, h, root, startDepth, config, hmin, vmin, outNodes);
}

const TreemapNode* TreemapEngine::HitTestItem(
	const std::vector<TreemapNode> &nodes,
	int px, int py)
{
	const TreemapNode *best = nullptr;

	for (const auto &cur : nodes) {
		if (px > cur.x && py > cur.y && px < cur.x + cur.w && py < cur.y + cur.h) {
			if (cur.IsFolder()) {
				if (px < cur.x + 3 || py < cur.y + 12 || px > cur.x + cur.w - 3 || py > cur.y + cur.h - 3) {
					best = &cur;
					break;
				}
			} else {
				best = &cur;
				break;
			}
		}
	}

	if (best != nullptr && (best->name == nullptr || best->name[0] == L'<')) {
		best = nullptr;
	}

	return best;
}

const TreemapNode* TreemapEngine::HitTestContainer(
	const std::vector<TreemapNode> &nodes,
	int px, int py)
{
	const TreemapNode *best = nullptr;
	int min_left = -32768, min_top = -32768, min_right = 32767, min_bottom = 32767;

	for (const auto &cur : nodes) {
		if (cur.IsFolder()
			&& px > cur.x && py > cur.y
			&& px < cur.x + cur.w && py < cur.y + cur.h
			&& cur.x >= min_left && cur.y >= min_top
			&& cur.x + cur.w <= min_right && cur.y + cur.h <= min_bottom) {
			min_left = cur.x;
			min_top = cur.y;
			min_right = cur.x + cur.w;
			min_bottom = cur.y + cur.h;
			best = &cur;
		}
	}

	return best;
}
