#include "AsyncScanEngine.h"
#include "Folder.h"
#include "DiskUsage.h"
#include "PathUtil.h"

#include <windows.h>
#include <algorithm>

AsyncScanEngine::AsyncScanEngine()
	: m_clusterMask(0)
	, m_aligned(false)
	, m_numThreads(1)
{
}

AsyncScanEngine::~AsyncScanEngine()
{
	Cancel();
	WaitForCompletion();
	if (m_rootFolder != nullptr) {
		delete m_rootFolder;
		m_rootFolder = nullptr;
	}
}

bool AsyncScanEngine::StartScan(const std::wstring& rootPath, ui64 clusterMask, bool aligned, unsigned int threadCount)
{
	if (m_running.load()) return false;

	m_rootPath = rootPath;
	m_clusterMask = clusterMask;
	m_aligned = aligned;

	if (threadCount == 0) {
		unsigned int hw = std::thread::hardware_concurrency();
		m_numThreads = hw > 0 ? (std::min)((std::max)(hw, 2u), 16u) : 4u;
	} else {
		m_numThreads = threadCount;
	}

	m_numFiles.store(0);
	m_numFolders.store(0);
	m_bytesScanned.store(0);
	m_cancelled.store(false);
	m_complete.store(false);
	m_stopWorkers.store(false);
	m_pendingTasks.store(0);

	{
		std::lock_guard<std::mutex> lock(m_pathMutex);
		m_latestPath = rootPath;
	}

	if (m_rootFolder != nullptr) {
		delete m_rootFolder;
		m_rootFolder = nullptr;
	}
	m_rootArena.Reset();

	m_rootFolder = new CFolder;

	m_workerArenas.clear();
	for (size_t i = 0; i < m_numThreads; ++i) {
		m_workerArenas.push_back(std::make_unique<CStringArena>());
	}

	m_running.store(true);
	m_activeWorkers.store(m_numThreads);

	// Start worker threads
	m_workers.clear();
	for (size_t i = 0; i < m_numThreads; ++i) {
		m_workers.emplace_back(&AsyncScanEngine::WorkerThread, this, i);
	}

	// Enqueue root task
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_taskQueue.push(ScanTask{m_rootFolder, m_rootPath, 0});
		m_pendingTasks.store(1);
	}
	m_queueCv.notify_one();

	return true;
}

void AsyncScanEngine::WorkerThread(size_t workerIndex)
{
	while (!m_stopWorkers.load()) {
		ScanTask task;
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_queueCv.wait(lock, [this]() {
				return m_stopWorkers.load() || !m_taskQueue.empty();
			});

			if (m_stopWorkers.load()) break;
			if (m_taskQueue.empty()) continue;

			task = m_taskQueue.front();
			m_taskQueue.pop();
		}

		if (!m_cancelled.load()) {
			std::wstring currentPath = task.path;
			ScanSubtree(workerIndex, task.targetFolder, currentPath, task.depth);
		}

		int remaining = --m_pendingTasks;
		if (remaining == 0) {
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_doneCv.notify_all();
		}
	}

	m_activeWorkers.fetch_sub(1);
}

void AsyncScanEngine::ScanSubtree(size_t workerIndex, CFolder* folder, std::wstring& path, unsigned int depth)
{
	if (m_cancelled.load() || folder == nullptr || depth > 128) return;

	WIN32_FIND_DATAW finddata;
	std::wstring::size_type baseLength = PathUtil::AppendComponent(path, L"*.*");

	HANDLE handle = FindFirstFileW(path.c_str(), &finddata);
	path.resize(baseLength);
	BOOL gotfile = (handle != INVALID_HANDLE_VALUE);

	struct FoundChildDir {
		std::wstring name;
		ui64 writeTime;
	};

	std::vector<FoundChildDir> subdirs;

	while (gotfile && !m_cancelled.load()) {
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (finddata.cFileName[0] == L'.' && (finddata.cFileName[1] == L'\0'
				|| (finddata.cFileName[1] == L'.' && finddata.cFileName[2] == L'\0'))) {
				goto next_file;
			}
		}

		// Skip name-surrogate reparse points (junctions, symlinks, mount points),
		// for files and directories alike, to prevent circular recursion loops and
		// double counting. Cloud placeholders (OneDrive etc.) are NOT name
		// surrogates and fall through to be scanned normally.
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			if (IsReparseTagNameSurrogate(finddata.dwReserved0)) {
				goto next_file;
			}
		}

		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			m_numFolders.fetch_add(1, std::memory_order_relaxed);

			FoundChildDir dir;
			dir.name = finddata.cFileName;
			dir.writeTime = *(ui64 *)&finddata.ftLastWriteTime;
			subdirs.push_back(dir);
		} else {
			m_numFiles.fetch_add(1, std::memory_order_relaxed);

			std::wstring::size_type fileLength = PathUtil::AppendComponent(path, finddata.cFileName);

			SM_FILE_SIZE_INFO sizeinfo;
			SM_LoadFileSizeInfoW(path.c_str(), &finddata, &sizeinfo);

			ui64 actualsize = (ui64)SM_GetLogicalFileSize(&sizeinfo);
			ui64 size = (ui64)SM_ChooseDisplayedFileSize(&sizeinfo, m_clusterMask, m_aligned);

			BOOL added;
			{
				std::lock_guard<std::mutex> lock(m_treeMutex);
				added = folder->AddFileWithArena(*m_workerArenas[workerIndex], finddata.cFileName,
					(ui32)wcslen(finddata.cFileName), size, actualsize, *(ui64 *)&finddata.ftLastWriteTime);
			}

			if (!added) {
				// Out of memory: abort the whole scan instead of silently dropping entries.
				Abort();
			} else {
				m_bytesScanned.fetch_add(size, std::memory_order_relaxed);
			}
			path.resize(fileLength);
		}

	next_file:
		gotfile = FindNextFileW(handle, &finddata);
	}

	if (handle != INVALID_HANDLE_VALUE) {
		FindClose(handle);
	}

	if (m_cancelled.load()) return;

	// Update reported path for UI
	{
		std::lock_guard<std::mutex> lock(m_pathMutex);
		m_latestPath = path;
	}

	// Process subdirectories. Each child is attached to its parent BEFORE it is
	// dispatched or scanned, so the tree owns every folder from the moment it
	// exists (nothing to clean up on cancellation) and no other thread can be
	// mutating a child while it gets attached.
	for (auto& dir : subdirs) {
		if (m_cancelled.load()) break;

		CFolder *child = new CFolder;
		BOOL added;
		{
			std::lock_guard<std::mutex> lock(m_treeMutex);
			added = folder->AddFolderWithArena(*m_workerArenas[workerIndex], dir.name.c_str(),
				(ui32)dir.name.length(), child, dir.writeTime);
		}

		if (!added) {
			// Out of memory: abort the whole scan instead of silently dropping the subtree.
			delete child;
			Abort();
			break;
		}

		std::wstring childPath = path;
		PathUtil::AppendComponent(childPath, dir.name.c_str());
		if (!childPath.empty() && childPath.back() != L'\\') {
			childPath += L'\\';
		}

		// Parallelize top directory levels
		bool dispatchedAsTask = false;
		if (depth < 3 && m_numThreads > 1) {
			std::lock_guard<std::mutex> lock(m_queueMutex);
			if (!m_stopWorkers.load() && m_taskQueue.size() < m_numThreads * 2) {
				m_pendingTasks.fetch_add(1);
				m_taskQueue.push(ScanTask{child, childPath, depth + 1});
				dispatchedAsTask = true;
			}
		}

		if (dispatchedAsTask) {
			m_queueCv.notify_one();
		} else {
			ScanSubtree(workerIndex, child, childPath, depth + 1);
		}
	}
}

void AsyncScanEngine::Abort()
{
	// The stop/cancel flags are set while holding the queue mutex so that a worker
	// checking its wait predicate can never miss the notify that follows (a notify
	// fired between the predicate check and blocking would otherwise be lost).
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_cancelled.store(true);
		m_stopWorkers.store(true);
	}
	m_queueCv.notify_all();
	m_doneCv.notify_all();
}

void AsyncScanEngine::Cancel()
{
	Abort();

	// Break any worker blocked inside FindFirstFile/FindNextFile on unresponsive
	// media (network shares, spun-down disks) so cancellation stays prompt. Only
	// the owning thread may do this: it touches the thread handles, which
	// WaitForCompletion (same thread) joins and clears.
	for (auto& worker : m_workers) {
		if (worker.joinable()) {
			CancelSynchronousIo(worker.native_handle());
		}
	}
}

void AsyncScanEngine::RequestStop()
{
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_stopWorkers.store(true);
	}
	m_queueCv.notify_all();
}

void AsyncScanEngine::WaitForCompletion()
{
	if (!m_running.load()) return;

	// Wait until all pending tasks are finished or cancelled
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_doneCv.wait(lock, [this]() {
			return m_pendingTasks.load() == 0 || m_cancelled.load();
		});

		// Stop workers; set under the mutex so no worker can miss the notify.
		m_stopWorkers.store(true);
	}
	m_queueCv.notify_all();

	for (auto& worker : m_workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	m_workers.clear();

	// A cancelled scan leaves undispatched tasks behind; their folders are
	// already attached to the tree, so the tasks can simply be dropped.
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		std::queue<ScanTask>().swap(m_taskQueue);
		m_pendingTasks.store(0);
	}

	if (!m_cancelled.load() && m_rootFolder != nullptr) {
		// Merge all thread arenas into root arena
		for (auto& arena : m_workerArenas) {
			m_rootArena.Merge(*arena);
		}
		m_rootFolder->Finalize();
		m_complete.store(true);
	}

	m_running.store(false);
}

ScanProgress AsyncScanEngine::GetProgress() const
{
	ScanProgress p;
	p.numFiles = m_numFiles.load(std::memory_order_relaxed);
	p.numFolders = m_numFolders.load(std::memory_order_relaxed);
	p.bytesScanned = m_bytesScanned.load(std::memory_order_relaxed);
	p.isComplete = m_complete.load(std::memory_order_relaxed);
	p.isCancelled = m_cancelled.load(std::memory_order_relaxed);

	{
		std::lock_guard<std::mutex> lock(m_pathMutex);
		p.currentPath = m_latestPath;
	}
	return p;
}

bool AsyncScanEngine::IsScanning() const
{
	return m_running.load() && !m_complete.load() && !m_cancelled.load() && (m_pendingTasks.load() > 0);
}

bool AsyncScanEngine::IsFinished() const
{
	return m_activeWorkers.load() == 0;
}

bool AsyncScanEngine::IsCancelled() const
{
	return m_cancelled.load();
}

CFolder* AsyncScanEngine::DetachResult(CStringArena& targetArena)
{
	std::lock_guard<std::mutex> lock(m_treeMutex);
	if (m_cancelled.load() || m_rootFolder == nullptr) return nullptr;

	targetArena.Merge(m_rootArena);
	CFolder* result = m_rootFolder;
	m_rootFolder = nullptr;
	return result;
}

static ui64 ComputeLiveSizes(CFolder* folder)
{
	if (folder == nullptr) return 0;

	ui64 total_children = 0;
	for (unsigned int i = 0; i < folder->cur; ++i) {
		if (folder->children[i] != nullptr) {
			ui64 childSize = ComputeLiveSizes(folder->children[i]);
			folder->sizes[i] = childSize;
			folder->actualsizes[i] = childSize;
			total_children += childSize;
		}
	}
	folder->size_children = total_children;
	return folder->size_self + total_children;
}

void AsyncScanEngine::GenerateLiveLayout(
	int w, int h,
	ui64 totalDiskSpace,
	ui64 freeDiskSpace,
	const TreemapConfig& config,
	std::vector<TreemapNode>& outNodes,
	std::vector<std::wstring>& outNameStorage)
{
	outNodes.clear();
	outNameStorage.clear();
	if (w <= 0 || h <= 0) return;

	std::lock_guard<std::mutex> lock(m_treeMutex);
	if (m_rootFolder == nullptr || m_cancelled.load()) return;

	ComputeLiveSizes(m_rootFolder);

	unsigned int rootCount = m_rootFolder->cur;
	if (rootCount == 0 && totalDiskSpace == 0) return;

	unsigned int extraItems = 0;
	if (freeDiskSpace > 0 && config.showFreeSpace) extraItems++;

	unsigned int totalItems = rootCount + extraItems;
	if (totalItems == 0) return;

	CFolder liveRoot;
	liveRoot.names = new (std::nothrow) wchar_t*[totalItems];
	liveRoot.children = new (std::nothrow) CFolder*[totalItems];
	liveRoot.sizes = new (std::nothrow) ui64[totalItems];
	liveRoot.actualsizes = new (std::nothrow) ui64[totalItems];
	liveRoot.times = new (std::nothrow) ui64[totalItems];

	if (!liveRoot.names || !liveRoot.children || !liveRoot.sizes || !liveRoot.actualsizes || !liveRoot.times) {
		delete[] liveRoot.names;
		delete[] liveRoot.children;
		delete[] liveRoot.sizes;
		delete[] liveRoot.actualsizes;
		delete[] liveRoot.times;
		return;
	}

	liveRoot.cur = 0;
	liveRoot.max = totalItems;

	for (unsigned int i = 0; i < rootCount; ++i) {
		liveRoot.names[liveRoot.cur] = m_rootFolder->names[i];
		liveRoot.children[liveRoot.cur] = m_rootFolder->children[i];
		liveRoot.sizes[liveRoot.cur] = m_rootFolder->sizes[i];
		liveRoot.actualsizes[liveRoot.cur] = m_rootFolder->actualsizes[i];
		liveRoot.times[liveRoot.cur] = m_rootFolder->times[i];
		liveRoot.cur++;
	}

	if (freeDiskSpace > 0 && config.showFreeSpace) {
		static const wchar_t freeName[] = L"<Free Space>";
		liveRoot.names[liveRoot.cur] = const_cast<wchar_t*>(freeName);
		liveRoot.children[liveRoot.cur] = nullptr;
		liveRoot.sizes[liveRoot.cur] = freeDiskSpace;
		liveRoot.actualsizes[liveRoot.cur] = freeDiskSpace;
		liveRoot.times[liveRoot.cur] = 0;
		liveRoot.cur++;
	}

	TreemapEngine::ComputeLayout(0, 0, w, h, &liveRoot, 0, config, outNodes);

	// Detach the snapshot from the live tree: workers keep mutating folder
	// arrays after this lock is released, and liveRoot dies with this frame.
	// Copying names and nulling source makes the nodes safe to keep and draw.
	// reserve() guarantees no reallocation, so c_str() pointers stay stable.
	outNameStorage.reserve(outNodes.size());
	for (auto& node : outNodes) {
		if (node.name != nullptr) {
			outNameStorage.emplace_back(node.name);
			node.name = outNameStorage.back().c_str();
		}
		node.source = nullptr;
		node.index = (ui32)-1;
	}

	// Cleanup without triggering CFolder recursive deletion
	delete[] liveRoot.names;
	delete[] liveRoot.sizes;
	delete[] liveRoot.actualsizes;
	delete[] liveRoot.times;
	delete[] liveRoot.children;
	liveRoot.names = nullptr;
	liveRoot.children = nullptr;
	liveRoot.sizes = nullptr;
	liveRoot.actualsizes = nullptr;
	liveRoot.times = nullptr;
	liveRoot.cur = 0;
	liveRoot.max = 0;
}
