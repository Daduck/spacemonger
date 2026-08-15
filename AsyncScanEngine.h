#ifndef ASYNCSCANENGINE_H
#define ASYNCSCANENGINE_H

#ifdef bool
#pragma push_macro("bool")
#undef bool
#endif

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "e.h"
#include "StringArena.h"
#include "TreemapLayout.h"

#ifdef bool
#pragma pop_macro("bool")
#endif

struct CFolder;
class CFolderTree;

struct ScanProgress {
	ui64 numFiles;
	ui64 numFolders;
	ui64 bytesScanned;
	std::wstring currentPath;
	bool isComplete;
	bool isCancelled;
};

class AsyncScanEngine {
public:
	AsyncScanEngine();
	~AsyncScanEngine();

	bool StartScan(const std::wstring& rootPath, ui64 clusterMask, bool aligned, unsigned int threadCount = 0);
	// Must be called from the thread that owns the engine (the one that called
	// StartScan); it touches the worker-thread handles to break blocking I/O.
	void Cancel();
	// Tells idle workers to exit once all tasks are done; does not block.
	void RequestStop();
	void WaitForCompletion();
	ScanProgress GetProgress() const;
	bool IsScanning() const;
	// True once every worker thread has returned (they may not be joined yet).
	bool IsFinished() const;
	bool IsCancelled() const;

	CFolder* DetachResult(CStringArena& targetArena);

	void GenerateLiveLayout(
		int w, int h,
		ui64 totalDiskSpace,
		ui64 freeDiskSpace,
		const TreemapConfig& config,
		std::vector<TreemapNode>& outNodes
	);

private:
	void WorkerThread(size_t workerIndex);
	void ScanSubtree(size_t workerIndex, CFolder* folder, std::wstring& path, unsigned int depth);
	// Flags-only cancellation, safe to call from worker threads.
	void Abort();

	std::wstring m_rootPath;
	ui64 m_clusterMask;
	bool m_aligned;
	unsigned int m_numThreads;

	std::vector<std::thread> m_workers;
	std::vector<std::unique_ptr<CStringArena>> m_workerArenas;

	struct ScanTask {
		CFolder* targetFolder;
		std::wstring path;
		unsigned int depth;
	};

	std::queue<ScanTask> m_taskQueue;
	mutable std::mutex m_queueMutex;
	std::condition_variable m_queueCv;
	std::condition_variable m_doneCv;

	std::atomic<int> m_pendingTasks{0};
	std::atomic<unsigned int> m_activeWorkers{0};
	std::atomic<bool> m_stopWorkers{false};
	std::atomic<bool> m_cancelled{false};
	std::atomic<bool> m_running{false};
	std::atomic<bool> m_complete{false};

	std::atomic<ui64> m_numFiles{0};
	std::atomic<ui64> m_numFolders{0};
	std::atomic<ui64> m_bytesScanned{0};

	mutable std::mutex m_pathMutex;
	std::wstring m_latestPath;

	mutable std::mutex m_treeMutex;

	CFolder* m_rootFolder{nullptr};
	CStringArena m_rootArena;
};

#endif // ASYNCSCANENGINE_H
