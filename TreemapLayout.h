#ifndef TREEMAPLAYOUT_H
#define TREEMAPLAYOUT_H

#include <vector>
#include <string>
#include "e.h"
#include "Folder.h"

enum TreemapNodeFlags : ui32 {
	TREEMAP_FLAG_NONE     = 0,
	TREEMAP_FLAG_FOLDER   = (1 << 0), // 1: Node represents a folder
	TREEMAP_FLAG_SPECIAL  = (1 << 1), // 2: Special block (e.g. Free Space)
	TREEMAP_FLAG_HOVER    = (1 << 2), // 4: Mouse hover highlight
};

struct TreemapNode {
	const wchar_t *name;
	si32 depth;
	ui32 flags;
	CFolder *source;
	ui32 index;
	si16 x, y, w, h;

	TreemapNode()
		: name(nullptr), depth(0), flags(TREEMAP_FLAG_NONE),
		  source(nullptr), index((ui32)-1),
		  x(0), y(0), w(0), h(0) {}

	inline bool IsFolder() const { return (flags & TREEMAP_FLAG_FOLDER) != 0; }
	inline bool IsSpecial() const { return (flags & TREEMAP_FLAG_SPECIAL) != 0; }
	inline bool IsHovered() const { return (flags & TREEMAP_FLAG_HOVER) != 0; }
};

struct TreemapConfig {
	int density;          // [-3..3], defaults to 0
	int bias;             // Aspect ratio preference [-8..8], defaults to 0
	bool showFreeSpace;   // Whether to allocate space for <Free Space>
	int dpi;              // Target DPI, defaults to 96

	TreemapConfig()
		: density(0), bias(0), showFreeSpace(true), dpi(96) {}
};

class TreemapEngine {
public:
	// Computes layout for the given folder tree into a flat vector of TreemapNode
	static void ComputeLayout(
		int x, int y, int w, int h,
		CFolder *root,
		int startDepth,
		const TreemapConfig &config,
		std::vector<TreemapNode> &outNodes
	);

	// Calculates DPI-scaled minimum box dimensions based on density setting [-3..3]
	static void GetMinDimensions(int density, int dpi, int &outHMin, int &outVMin);

	// Finds the innermost item (file or folder header) at point (px, py)
	static const TreemapNode* HitTestItem(
		const std::vector<TreemapNode> &nodes,
		int px, int py
	);

	// Finds the tightest container folder enclosing point (px, py)
	static const TreemapNode* HitTestContainer(
		const std::vector<TreemapNode> &nodes,
		int px, int py
	);
};

#endif // TREEMAPLAYOUT_H
