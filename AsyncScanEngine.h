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
#ifdef ASYNC_SCAN_BENCHMARK
#include <array>
#endif
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
	// Terminal outcomes: complete (no omissions), partial (usable result),
	// failed (no usable result), or cancelled by the owner.
	bool isComplete;
	bool isCancelled;
	bool isPartial;
	bool isFailed;
	// Counts directly skipped/interrupted directories, not their descendants.
	ui64 skippedDirectories;
	unsigned long firstError;
};

#ifdef ASYNC_SCAN_BENCHMARK
struct TreeLockMetricSnapshot {
	ui64 count{0};
	ui64 waitNanoseconds{0};
	ui64 maxWaitNanoseconds{0};
	ui64 holdNanoseconds{0};
	ui64 maxHoldNanoseconds{0};
	ui64 p95WaitNanoseconds{0};
	ui64 p95HoldNanoseconds{0};
};
#endif

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
	const std::vector<std::wstring>& GetSkippedDirectories() const;
	bool IsScanning() const;
	// True once every worker thread has returned (they may not be joined yet).
	bool IsFinished() const;
	bool IsCancelled() const;

	CFolder* DetachResult(CStringArena& targetArena);

	// Produces a self-contained snapshot layout of the partially scanned tree.
	// The returned nodes never point back into the live tree (source is null,
	// names are copied into outNameStorage), so callers may keep and render
	// them without holding any lock, even while workers keep scanning.
	void GenerateLiveLayout(
		int w, int h,
		ui64 totalDiskSpace,
		ui64 freeDiskSpace,
		const TreemapConfig& config,
		std::vector<TreemapNode>& outNodes,
		std::vector<std::wstring>& outNameStorage
	);
#ifdef ASYNC_SCAN_BENCHMARK
	unsigned int GetBenchmarkWorkerCount() const { return m_numThreads; }
	TreeLockMetricSnapshot GetWorkerMutationLockMetrics() const;
	TreeLockMetricSnapshot GetLayoutLockMetrics() const;
#endif

private:
	void WorkerThread(size_t workerIndex);
	void ScanSubtree(size_t workerIndex, CFolder* folder, std::wstring& path, unsigned int depth);
	// Flags-only cancellation, safe to call from worker threads.
	void Abort(bool failed = false);
	void RecordScanError(unsigned long error, bool rootFailure, const std::wstring& path);

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
	std::atomic<bool> m_failed{false};
	std::atomic<ui64> m_skippedDirectories{0};
	std::atomic<unsigned long> m_firstError{0};
	static const size_t kMaxSkippedPaths = 4096;
	std::vector<std::wstring> m_skippedPaths;
	mutable std::mutex m_diagnosticsMutex;

	std::atomic<ui64> m_numFiles{0};
	std::atomic<ui64> m_numFolders{0};
	std::atomic<ui64> m_bytesScanned{0};

	mutable std::mutex m_pathMutex;
	std::wstring m_latestPath;

	mutable std::mutex m_treeMutex;

#ifdef ASYNC_SCAN_BENCHMARK
	struct BenchmarkLockMetrics {
		ui64 count{0}, waitNanoseconds{0}, maxWaitNanoseconds{0};
		ui64 holdNanoseconds{0}, maxHoldNanoseconds{0};
		std::array<ui64, 64> waitHistogram{};
		std::array<ui64, 64> holdHistogram{};
	};
	static TreeLockMetricSnapshot ReadMetrics(const BenchmarkLockMetrics& metrics);
	enum class LockMetricKind { WorkerMutation, Layout };
	class MeasuredTreeLock {
	public:
		MeasuredTreeLock(AsyncScanEngine& engine, LockMetricKind kind, size_t workerIndex = 0);
		~MeasuredTreeLock();
		MeasuredTreeLock(const MeasuredTreeLock&) = delete;
		MeasuredTreeLock& operator=(const MeasuredTreeLock&) = delete;
	private:
		std::unique_lock<std::mutex> m_lock;
		BenchmarkLockMetrics* m_metrics;
		bool m_measure{false};
		ui64 m_waitNanoseconds{0};
		ui64 m_acquiredAt{0};
	};
	std::vector<BenchmarkLockMetrics> m_workerMutationMetrics;
	std::vector<ui64> m_workerMutationSequences;
	BenchmarkLockMetrics m_layoutMetrics;
#endif

	CFolder* m_rootFolder{nullptr};
	CStringArena m_rootArena;
};

#endif // ASYNCSCANENGINE_H
