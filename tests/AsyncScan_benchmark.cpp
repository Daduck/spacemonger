#include "../AsyncScanEngine.h"
#include "../PathUtil.h"

#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr unsigned int kDirectories = 2000;
constexpr unsigned int kFilesPerDirectory = 1000;
constexpr unsigned int kDefaultSamples = 5;
constexpr unsigned int kRenderIntervalMs = 250;

struct Options {
	unsigned int samples{kDefaultSamples};
	unsigned int directories{kDirectories};
	unsigned int filesPerDirectory{kFilesPerDirectory};
};

struct Manifest {
	ui64 files{0};
	ui64 folders{0};
	ui64 logicalBytes{0};
};

class Fixture {
public:
	~Fixture()
	{
		if (!Cleanup()) std::fprintf(stderr, "could not completely clean up benchmark fixture %ls\n", m_root.c_str());
	}

	bool Cleanup() noexcept
	{
		try {
		if (m_reservedFile) {
			if (!DeleteFileW(m_root.c_str())) return false;
			m_reservedFile = false;
			return true;
		}
		if (!m_owned) return true;
		std::vector<std::wstring> directories;
		std::wstring query = m_root + L"\\*";
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW(query.c_str(), &data);
		if (find == INVALID_HANDLE_VALUE) return false;
		bool removed = true;
		do {
			if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
			std::wstring child = m_root + L"\\" + data.cFileName;
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
					if (!RemoveDirectoryW(child.c_str())) removed = false;
				} else {
					directories.push_back(std::move(child));
				}
			} else if (!DeleteFileW(child.c_str())) {
				removed = false;
			}
		} while (FindNextFileW(find, &data));
		DWORD enumerationError = GetLastError();
		FindClose(find);
		if (enumerationError != ERROR_NO_MORE_FILES) removed = false;

		std::atomic<size_t> nextDirectory{0};
		std::atomic<bool> directoryRemovalFailed{false};
		unsigned int hardwareThreads = std::thread::hardware_concurrency();
		unsigned int workerCount = (std::min)((std::max)(hardwareThreads, 2u), 16u);
		workerCount = (std::min)(workerCount, (std::max)((unsigned int)directories.size(), 1u));
		std::vector<std::thread> workers;
		try {
			for (unsigned int worker = 0; worker < workerCount; ++worker) {
				workers.emplace_back([&]() {
					try {
						for (;;) {
							size_t index = nextDirectory.fetch_add(1);
							if (index >= directories.size()) return;
							if (!RemoveContents(directories[index]) || !RemoveDirectoryW(directories[index].c_str()))
								directoryRemovalFailed.store(true);
						}
					} catch (...) { directoryRemovalFailed.store(true); }
				});
			}
		} catch (...) {
			for (auto& worker : workers) if (worker.joinable()) worker.join();
			return false;
		}
		for (auto& worker : workers) worker.join();
		if (directoryRemovalFailed.load()) removed = false;
		if (!RemoveDirectoryW(m_root.c_str())) removed = false;
		if (removed) m_owned = false;
		return removed;
		} catch (...) {
			return false;
		}
	}

	void Create(unsigned int directoryCount, unsigned int filesPerDirectory)
	{
		wchar_t tempPath[MAX_PATH];
		DWORD tempLength = GetTempPathW(MAX_PATH, tempPath);
		if (tempLength == 0 || tempLength >= MAX_PATH)
			throw std::runtime_error("GetTempPathW failed");
		wchar_t uniquePath[MAX_PATH];
		if (GetTempFileNameW(tempPath, L"SMB", 0, uniquePath) == 0)
			throw std::runtime_error("GetTempFileNameW failed");
		m_root = uniquePath;
		m_reservedFile = true;
		if (!DeleteFileW(m_root.c_str()))
			throw std::runtime_error("could not reserve a unique fixture path");
		m_reservedFile = false;
		if (!CreateDirectoryW(m_root.c_str(), nullptr))
			throw std::runtime_error("could not create the reserved fixture directory");
		m_owned = true;
		for (unsigned int d = 0; d < directoryCount; ++d) {
			wchar_t directoryName[32];
			swprintf_s(directoryName, L"dir-%03u", d);
			std::wstring directory = m_root + L"\\" + directoryName;
			if (!CreateDirectoryW(directory.c_str(), nullptr))
				throw std::runtime_error("could not create fixture subdirectory");
			++m_manifest.folders;
		}

		unsigned int hardwareThreads = std::thread::hardware_concurrency();
		unsigned int workerCount = (std::min)((std::max)(hardwareThreads, 2u), 16u);
		workerCount = (std::min)(workerCount, (std::max)(directoryCount, 1u));
		std::atomic<unsigned int> nextDirectory{0};
		std::atomic<bool> failed{false};
		std::vector<Manifest> workerManifests(workerCount);
		std::exception_ptr workerFailure;
		std::mutex failureMutex;
		std::vector<std::thread> workers;
		try {
			for (unsigned int worker = 0; worker < workerCount; ++worker) {
				workers.emplace_back([&, worker]() {
					try {
						for (;;) {
							if (failed.load()) return;
						unsigned int d = nextDirectory.fetch_add(1);
						if (d >= directoryCount) return;
						wchar_t directoryName[32];
						swprintf_s(directoryName, L"dir-%03u", d);
						std::wstring directory = m_root + L"\\" + directoryName;
						for (unsigned int f = 0; f < filesPerDirectory; ++f) {
							wchar_t fileName[32];
							swprintf_s(fileName, L"file-%03u.bin", f);
							const std::wstring filePath = directory + L"\\" + fileName;
							const DWORD length = 1 + ((d * 131u + f * 17u + d * f * 3u) % 4096u);
							CreateSizedFile(filePath, length);
							++workerManifests[worker].files;
							workerManifests[worker].logicalBytes += length;
						}
					}
					} catch (...) {
						failed.store(true);
						std::lock_guard<std::mutex> lock(failureMutex);
						if (!workerFailure) workerFailure = std::current_exception();
					}
				});
			}
		} catch (...) {
			failed.store(true);
			for (auto& worker : workers) if (worker.joinable()) worker.join();
			throw;
		}
		for (auto& worker : workers) worker.join();
		if (workerFailure) std::rethrow_exception(workerFailure);
		for (const auto& manifest : workerManifests) {
			m_manifest.files += manifest.files;
			m_manifest.folders += manifest.folders;
			m_manifest.logicalBytes += manifest.logicalBytes;
		}
	}

	const std::wstring& Root() const { return m_root; }
	const Manifest& Expected() const { return m_manifest; }

private:
	static void CreateSizedFile(const std::wstring& path, DWORD length)
	{
		HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			throw std::runtime_error("could not create fixture file");
		FILE_END_OF_FILE_INFO endOfFile{};
		endOfFile.EndOfFile.QuadPart = length;
		BOOL ok = SetFileInformationByHandle(file, FileEndOfFileInfo, &endOfFile, sizeof(endOfFile));
		CloseHandle(file);
		if (!ok) {
			DeleteFileW(path.c_str());
			throw std::runtime_error("could not set fixture file length");
		}
	}

	static bool RemoveContents(const std::wstring& path)
	{
		std::wstring query = path + L"\\*";
		WIN32_FIND_DATAW data;
		HANDLE find = FindFirstFileW(query.c_str(), &data);
		if (find == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
		bool removed = true;
		do {
			if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
			std::wstring child = path + L"\\" + data.cFileName;
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 && !RemoveContents(child)) removed = false;
				if (!RemoveDirectoryW(child.c_str())) removed = false;
			} else {
				if (!DeleteFileW(child.c_str())) removed = false;
			}
		} while (FindNextFileW(find, &data));
		DWORD enumerationError = GetLastError();
		FindClose(find);
		return removed && enumerationError == ERROR_NO_MORE_FILES;
	}

	std::wstring m_root;
	Manifest m_manifest;
	bool m_owned{false};
	bool m_reservedFile{false};
};

struct Sample {
	std::wstring mode;
	unsigned int index{0};
	unsigned int workers{0};
	double seconds{0};
	ui64 files{0}, folders{0}, bytes{0};
	ui64 workerMutationAcquisitions{0};
	ui64 layoutCalls{0};
	ui64 nonEmptyLayoutCalls{0};
	TreeLockMetricSnapshot workerLock, layoutLock;
	struct LayoutObservation {
		ui64 files{0}, folders{0}, bytes{0};
		size_t treemapNodes{0};
		bool populated{false};
	};
	std::vector<LayoutObservation> layoutObservations;
};

std::wstring ReadCpuIdentifier()
{
	wchar_t value[512];
	DWORD length = GetEnvironmentVariableW(L"PROCESSOR_IDENTIFIER", value, 512);
	return length > 0 && length < 512 ? std::wstring(value) : L"unavailable";
}

std::wstring ReadOsVersion()
{
	using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	RtlGetVersionFn getVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	if (!getVersion || getVersion(&version) != 0) return L"Windows version unavailable";
	return L"Windows " + std::to_wstring(version.dwMajorVersion) + L"." +
		std::to_wstring(version.dwMinorVersion) + L" build " + std::to_wstring(version.dwBuildNumber);
}

static ui64 AvailableTempBytes()
{
	wchar_t tempPath[MAX_PATH];
	DWORD length = GetTempPathW(MAX_PATH, tempPath);
	if (length == 0 || length >= MAX_PATH) throw std::runtime_error("could not read temp volume path");
	ULARGE_INTEGER available{};
	ULARGE_INTEGER total{};
	ULARGE_INTEGER free{};
	if (!GetDiskFreeSpaceExW(tempPath, &available, &total, &free))
		throw std::runtime_error("could not query temp volume free space");
	return available.QuadPart;
}

static double NanosecondsToMilliseconds(ui64 value) { return (double)value / 1000000.0; }

static void PrintLock(const char* label, const TreeLockMetricSnapshot& metrics)
{
	double meanWait = metrics.count ? NanosecondsToMilliseconds(metrics.waitNanoseconds) / metrics.count : 0;
	double meanHold = metrics.count ? NanosecondsToMilliseconds(metrics.holdNanoseconds) / metrics.count : 0;
	std::printf("  %s lock: count=%llu wait_ms_total=%.3f wait_ms_mean=%.6f wait_ms_max=%.3f wait_ms_p95_bucket<=%.6f hold_ms_total=%.3f hold_ms_mean=%.6f hold_ms_max=%.3f hold_ms_p95_bucket<=%.6f\n",
		label, (unsigned long long)metrics.count,
		NanosecondsToMilliseconds(metrics.waitNanoseconds), meanWait,
		NanosecondsToMilliseconds(metrics.maxWaitNanoseconds), NanosecondsToMilliseconds(metrics.p95WaitNanoseconds),
		NanosecondsToMilliseconds(metrics.holdNanoseconds), meanHold,
		NanosecondsToMilliseconds(metrics.maxHoldNanoseconds), NanosecondsToMilliseconds(metrics.p95HoldNanoseconds));
}

static void CheckManifest(const Sample& sample, const Manifest& expected)
{
	if (sample.files != expected.files || sample.folders != expected.folders || sample.bytes != expected.logicalBytes) {
			std::fprintf(stderr, "accounting mismatch in sample %u (%ls): files %llu/%llu, folders %llu/%llu, logical bytes %llu/%llu\n",
				sample.index, sample.mode.c_str(), (unsigned long long)sample.files, (unsigned long long)expected.files,
			(unsigned long long)sample.folders, (unsigned long long)expected.folders,
			(unsigned long long)sample.bytes, (unsigned long long)expected.logicalBytes);
		throw std::runtime_error("scan accounting did not match fixture manifest");
	}
}

static Sample RunSample(const Fixture& fixture, unsigned int sampleIndex, bool liveLayout)
{
	AsyncScanEngine engine;
	std::wstring root = PathUtil::EnsureTrailingBackslash(PathUtil::PrepareLongPath(fixture.Root()));
	std::vector<TreemapNode> nodes;
	std::vector<std::wstring> names;
	TreemapConfig config;
	ui64 layoutCalls = 0;
	ui64 nonEmptyLayoutCalls = 0;
	std::vector<Sample::LayoutObservation> layoutObservations;
	layoutObservations.reserve(16);
	ScanProgress lastRendered{};
	lastRendered.numFiles = (ui64)-1;
	lastRendered.numFolders = (ui64)-1;
	lastRendered.bytesScanned = (ui64)-1;
	auto started = std::chrono::steady_clock::now();
	// FolderTree initializes last_render_tick to zero, so the first active-loop
	// iteration is immediately eligible; subsequent ticks are 250 ms apart.
	auto nextRender = started;
	if (!engine.StartScan(root, 0, false, 0)) throw std::runtime_error("StartScan failed");
	const unsigned int workers = engine.GetBenchmarkWorkerCount();
	for (;;) {
		if (!engine.IsScanning()) break;
		ScanProgress progress = engine.GetProgress();
		auto now = std::chrono::steady_clock::now();
		if (liveLayout && now >= nextRender) {
			nextRender = now + std::chrono::milliseconds(kRenderIntervalMs);
			ui64 entries = progress.numFiles + progress.numFolders;
			ui64 previousEntries = lastRendered.numFiles + lastRendered.numFolders;
			if (progress.bytesScanned != lastRendered.bytesScanned || entries != previousEntries) {
				lastRendered = progress;
				engine.GenerateLiveLayout(1280, 800, 0, 0, config, nodes, names);
				++layoutCalls;
				bool nonEmpty = !nodes.empty();
				if (nonEmpty) ++nonEmptyLayoutCalls;
				layoutObservations.push_back({progress.numFiles, progress.numFolders,
					progress.bytesScanned, nodes.size(), nonEmpty});
			}
		}
		Sleep(15);
	}
	engine.RequestStop();
	engine.WaitForCompletion();
	auto finished = std::chrono::steady_clock::now();
	ScanProgress progress = engine.GetProgress();
	if (!progress.isComplete || progress.isCancelled || progress.isFailed || progress.skippedDirectories != 0)
		throw std::runtime_error("scan did not complete successfully");

	Sample sample;
	sample.mode = liveLayout ? L"live-layout" : L"scan-only";
	sample.index = sampleIndex;
	sample.workers = workers;
	sample.seconds = std::chrono::duration<double>(finished - started).count();
	sample.files = progress.numFiles;
	sample.folders = progress.numFolders;
	sample.bytes = progress.bytesScanned;
	// Each discovered file and folder is inserted under exactly one worker
	// mutation lock. This derives total acquisitions from verified scan progress.
	sample.workerMutationAcquisitions = progress.numFiles + progress.numFolders;
	sample.layoutCalls = layoutCalls;
	sample.nonEmptyLayoutCalls = nonEmptyLayoutCalls;
	sample.layoutObservations = std::move(layoutObservations);
	sample.workerLock = engine.GetWorkerMutationLockMetrics();
	sample.layoutLock = engine.GetLayoutLockMetrics();
	if (sample.layoutLock.count != sample.layoutCalls)
		throw std::runtime_error("layout lock metric count did not match GenerateLiveLayout calls");
	if (workers != engine.GetBenchmarkWorkerCount()) throw std::runtime_error("worker count changed within scan");
	return sample;
}

static double Median(std::vector<double> values)
{
	std::sort(values.begin(), values.end());
	if (values.empty()) return 0;
	return values.size() % 2 ? values[values.size() / 2] : (values[values.size() / 2 - 1] + values[values.size() / 2]) / 2.0;
}

static ui64 UpperMedianU64(std::vector<ui64> values)
{
	std::sort(values.begin(), values.end());
	if (values.empty()) return 0;
	return values[values.size() / 2];
}

static void VerifyUpperMedianSemantics()
{
	if (UpperMedianU64({1, 2, 8, 16}) != 8 || UpperMedianU64({1, 2, 8}) != 2)
		throw std::logic_error("upper-median percentile selection check failed");
}

static void PrintLayoutObservations(const Sample& sample)
{
	for (size_t i = 0; i < sample.layoutObservations.size(); ++i) {
		const auto& observation = sample.layoutObservations[i];
		std::printf("Layout sample=%u mode=%ls call=%zu progress_files=%llu progress_folders=%llu progress_bytes=%llu treemap_nodes=%zu populated=%s\n",
			sample.index, sample.mode.c_str(), i + 1,
			(unsigned long long)observation.files, (unsigned long long)observation.folders,
			(unsigned long long)observation.bytes, observation.treemapNodes,
			observation.populated ? "yes" : "no");
	}
}

static void PrintSummary(const char* label, const std::vector<Sample>& samples, bool live)
{
	std::vector<double> wallSeconds, filesPerSecond, bytesPerSecond;
	ui64 layoutCalls = 0, nonEmptyLayoutCalls = 0, workerMutationAcquisitions = 0;
	TreeLockMetricSnapshot worker, layout;
	std::vector<ui64> workerWaitP95, workerHoldP95, layoutWaitP95, layoutHoldP95;
	for (const auto& sample : samples) {
		if ((sample.mode == L"live-layout") != live) continue;
		wallSeconds.push_back(sample.seconds);
		filesPerSecond.push_back(sample.files / sample.seconds);
		bytesPerSecond.push_back(sample.bytes / sample.seconds);
		layoutCalls += sample.layoutCalls;
		nonEmptyLayoutCalls += sample.nonEmptyLayoutCalls;
		workerMutationAcquisitions += sample.workerMutationAcquisitions;
		auto add = [](TreeLockMetricSnapshot& into, const TreeLockMetricSnapshot& from) {
			into.count += from.count; into.waitNanoseconds += from.waitNanoseconds; into.holdNanoseconds += from.holdNanoseconds;
			into.maxWaitNanoseconds = (std::max)(into.maxWaitNanoseconds, from.maxWaitNanoseconds);
			into.maxHoldNanoseconds = (std::max)(into.maxHoldNanoseconds, from.maxHoldNanoseconds);
		};
		add(worker, sample.workerLock); add(layout, sample.layoutLock);
		workerWaitP95.push_back(sample.workerLock.p95WaitNanoseconds);
		workerHoldP95.push_back(sample.workerLock.p95HoldNanoseconds);
		layoutWaitP95.push_back(sample.layoutLock.p95WaitNanoseconds);
		layoutHoldP95.push_back(sample.layoutLock.p95HoldNanoseconds);
	}
	worker.p95WaitNanoseconds = UpperMedianU64(workerWaitP95);
	worker.p95HoldNanoseconds = UpperMedianU64(workerHoldP95);
	layout.p95WaitNanoseconds = UpperMedianU64(layoutWaitP95);
	layout.p95HoldNanoseconds = UpperMedianU64(layoutHoldP95);
	std::printf("%s median: wall_seconds=%.3f files_per_sec=%.0f bytes_per_sec=%.0f worker_mutation_acquisitions_total=%llu worker_mutations_sampled=%llu sampling=1/256 layout_calls_total=%llu non_empty_layout_calls=%llu\n", label,
		Median(wallSeconds), Median(filesPerSecond), Median(bytesPerSecond), (unsigned long long)workerMutationAcquisitions,
		(unsigned long long)worker.count, (unsigned long long)layoutCalls, (unsigned long long)nonEmptyLayoutCalls);
	PrintLock("worker-mutation aggregate", worker);
	PrintLock("layout aggregate", layout);
}

static Options ParseOptions(int argc, wchar_t** argv)
{
	Options options;
	for (int i = 1; i < argc; ++i) {
		if (i + 1 >= argc) throw std::runtime_error("usage: AsyncScan_benchmark.exe [--samples N] [--directories N] [--files-per-directory N]");
		const wchar_t* key = argv[i++];
		wchar_t* end = nullptr;
		unsigned long value = wcstoul(argv[i], &end, 10);
		if (!end || *end || value == 0 || value > 1000000) throw std::runtime_error("numeric options must be positive integers");
		if (wcscmp(key, L"--samples") == 0 && value <= 100) options.samples = (unsigned int)value;
		else if (wcscmp(key, L"--directories") == 0) options.directories = (unsigned int)value;
		else if (wcscmp(key, L"--files-per-directory") == 0) options.filesPerDirectory = (unsigned int)value;
		else throw std::runtime_error("unknown option or out-of-range samples value");
	}
	if ((ui64)options.directories * options.filesPerDirectory > 2000000)
		throw std::runtime_error("fixture is limited to 2,000,000 files");
	return options;
}
}

int wmain(int argc, wchar_t** argv)
{
	try {
		VerifyUpperMedianSemantics();
		const Options options = ParseOptions(argc, argv);
		Fixture fixture;
		ui64 freeBeforeFixture = AvailableTempBytes();
		auto fixtureStart = std::chrono::steady_clock::now();
		fixture.Create(options.directories, options.filesPerDirectory);
		double fixtureSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - fixtureStart).count();
		ui64 freeAfterFixture = AvailableTempBytes();
		SYSTEM_INFO systemInfo{};
		GetNativeSystemInfo(&systemInfo);
#ifdef _DEBUG
		const char* buildType = "Debug";
#else
		const char* buildType = "Release";
#endif
		std::printf("Host: %ls | CPU: %ls | logical_processors=%lu | build=%s\n",
			ReadOsVersion().c_str(), ReadCpuIdentifier().c_str(), systemInfo.dwNumberOfProcessors, buildType);
		std::printf("Fixture: %u directories x %u files, creation_seconds=%.3f manifest files=%llu folders=%llu logical_bytes=%llu\n",
			options.directories, options.filesPerDirectory, fixtureSeconds, (unsigned long long)fixture.Expected().files,
			(unsigned long long)fixture.Expected().folders, (unsigned long long)fixture.Expected().logicalBytes);
		std::printf("Temp volume available bytes before=%llu after_create=%llu approximate_fixture_delta=%lld\n",
			(unsigned long long)freeBeforeFixture, (unsigned long long)freeAfterFixture,
			(long long)freeBeforeFixture - (long long)freeAfterFixture);
		std::printf("Runs: %u per mode; order alternates by pair; live-layout cadence=250 ms with progress change check; workers=default\n",
			options.samples);

		std::vector<Sample> results;
		results.reserve(options.samples * 2);
		bool liveFirst = false;
		for (unsigned int i = 0; i < options.samples; ++i) {
			bool firstIsLive = liveFirst;
			for (unsigned int order = 0; order < 2; ++order) {
				bool live = order == 0 ? firstIsLive : !firstIsLive;
				Sample sample = RunSample(fixture, (unsigned int)results.size() + 1, live);
				CheckManifest(sample, fixture.Expected());
				if (live && sample.layoutCalls == 0)
					throw std::runtime_error("live-layout sample had zero cadence-triggered layouts; increase the fixture size");
				if (live && sample.nonEmptyLayoutCalls == 0)
					throw std::runtime_error("live-layout sample had no non-empty cadence-triggered layout; increase the fixture size");
				PrintLayoutObservations(sample);
				double filesPerSecond = sample.files / sample.seconds;
				double bytesPerSecond = sample.bytes / sample.seconds;
				std::printf("Sample %u %s: workers=%u wall_seconds=%.3f files_per_sec=%.0f bytes_per_sec=%.0f files=%llu folders=%llu logical_bytes=%llu worker_mutation_acquisitions_total=%llu worker_mutations_sampled=%llu sampling=1/256 layout_calls=%llu non_empty_layout_calls=%llu\n",
					sample.index, live ? "live-layout" : "scan-only", sample.workers, sample.seconds, filesPerSecond, bytesPerSecond,
					(unsigned long long)sample.files, (unsigned long long)sample.folders,
					(unsigned long long)sample.bytes, (unsigned long long)sample.workerMutationAcquisitions,
					(unsigned long long)sample.workerLock.count, (unsigned long long)sample.layoutCalls,
					(unsigned long long)sample.nonEmptyLayoutCalls);
				PrintLock("worker-mutation", sample.workerLock);
				PrintLock("layout", sample.layoutLock);
				results.push_back(sample);
			}
			liveFirst = !liveFirst;
		}
		PrintSummary("scan-only", results, false);
		PrintSummary("live-layout", results, true);
		std::vector<double> scanRates, liveRates, scanByteRates, liveByteRates;
		for (const auto& sample : results) {
			if (sample.mode == L"live-layout") {
				liveRates.push_back(sample.files / sample.seconds);
				liveByteRates.push_back(sample.bytes / sample.seconds);
			} else {
				scanRates.push_back(sample.files / sample.seconds);
				scanByteRates.push_back(sample.bytes / sample.seconds);
			}
		}
		double scanMedian = Median(scanRates), liveMedian = Median(liveRates);
		double scanByteMedian = Median(scanByteRates), liveByteMedian = Median(liveByteRates);
		double fileChange = scanMedian == 0 ? 0 : (liveMedian - scanMedian) * 100.0 / scanMedian;
		double byteChange = scanByteMedian == 0 ? 0 : (liveByteMedian - scanByteMedian) * 100.0 / scanByteMedian;
		std::printf("Median throughput change with live layout: files_per_sec=%.2f%% bytes_per_sec=%.2f%%\n", fileChange, byteChange);
		auto cleanupStart = std::chrono::steady_clock::now();
		if (!fixture.Cleanup()) throw std::runtime_error("could not completely clean up the benchmark fixture");
		double cleanupSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - cleanupStart).count();
		ui64 freeAfterCleanup = AvailableTempBytes();
		std::printf("Fixture cleanup_seconds=%.3f temp_volume_available_after_cleanup=%llu\n",
			cleanupSeconds, (unsigned long long)freeAfterCleanup);
		return 0;
	} catch (const std::exception& error) {
		std::fprintf(stderr, "benchmark failed: %s\n", error.what());
		return 1;
	}
}
