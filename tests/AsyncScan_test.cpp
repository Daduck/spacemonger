#include "../AsyncScanEngine.h"
#include "../FolderTree.h"
#include "../DiskUsage.h"
#include "../PathUtil.h"

#include <windows.h>
#include <stdio.h>
#include <string>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static std::wstring CreateTempTestDirectory()
{
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);
	wchar_t uniqueName[MAX_PATH];
	GetTempFileNameW(tempPath, L"SMT", 0, uniqueName);
	DeleteFileW(uniqueName);
	CreateDirectoryW(uniqueName, NULL);
	return std::wstring(uniqueName);
}

static void DeleteTestDirectory(const std::wstring& path)
{
	std::wstring searchPath = path + L"\\*.*";
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
				continue;
			std::wstring childPath = path + L"\\" + fd.cFileName;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteTestDirectory(childPath);
			} else {
				DeleteFileW(childPath.c_str());
			}
		} while (FindNextFileW(h, &fd));
		FindClose(h);
	}
	RemoveDirectoryW(path.c_str());
}

static void CreateDummyFile(const std::wstring& filePath, size_t sizeBytes)
{
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (sizeBytes > 0) {
			std::vector<char> buffer(sizeBytes, 'A');
			DWORD written = 0;
			WriteFile(hFile, buffer.data(), (DWORD)buffer.size(), &written, NULL);
		}
		CloseHandle(hFile);
	}
}

static int test_async_scan_directory_tree()
{
	std::wstring tempDir = CreateTempTestDirectory();

	// Create structure:
	// tempDir/
	//   file1.txt (100 bytes)
	//   file2.txt (200 bytes)
	//   sub1/
	//     file3.txt (300 bytes)
	//     sub2/
	//       file4.txt (400 bytes)
	CreateDummyFile(tempDir + L"\\file1.txt", 100);
	CreateDummyFile(tempDir + L"\\file2.txt", 200);

	std::wstring sub1 = tempDir + L"\\sub1";
	CreateDirectoryW(sub1.c_str(), NULL);
	CreateDummyFile(sub1 + L"\\file3.txt", 300);

	std::wstring sub2 = sub1 + L"\\sub2";
	CreateDirectoryW(sub2.c_str(), NULL);
	CreateDummyFile(sub2 + L"\\file4.txt", 400);

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 0, false, 4));
	engine.WaitForCompletion();

	CHECK(!engine.IsCancelled());
	ScanProgress progress = engine.GetProgress();
	CHECK(progress.isComplete);
	CHECK(progress.numFiles == 4);
	CHECK(progress.numFolders == 2);

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	CHECK(root->cur >= 2); // file1, file2, sub1
	CHECK(root->SizeTotal() == 1000);
	CHECK(root->SizeSub() == 700); // sub1 (file3: 300 + sub2/file4: 400)
	CHECK(root->SizeFiles() == 300); // file1: 100 + file2: 200

	delete root;
	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_async_scan_cancellation()
{
	std::wstring tempDir = CreateTempTestDirectory();
	for (int i = 0; i < 20; ++i) {
		std::wstring sub = tempDir + L"\\dir" + std::to_wstring(i);
		CreateDirectoryW(sub.c_str(), NULL);
		CreateDummyFile(sub + L"\\file.txt", 50);
	}

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 4095, true, 2));
	engine.Cancel();
	engine.WaitForCompletion();

	CHECK(engine.IsCancelled());

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root == NULL);

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_async_scan_rescan_after_cancel()
{
	// Regression test: a cancelled scan must not leave stale tasks or a stranded
	// pending-task counter behind that would corrupt or hang a subsequent scan
	// on the same engine instance.
	std::wstring tempDir = CreateTempTestDirectory();
	for (int i = 0; i < 20; ++i) {
		std::wstring sub = tempDir + L"\\dir" + std::to_wstring(i);
		CreateDirectoryW(sub.c_str(), NULL);
		CreateDummyFile(sub + L"\\file.txt", 50);
	}

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 0, false, 2));
	engine.Cancel();
	engine.WaitForCompletion();
	CHECK(engine.IsCancelled());

	CHECK(engine.StartScan(preparedPath, 0, false, 2));
	engine.WaitForCompletion();
	CHECK(!engine.IsCancelled());

	ScanProgress progress = engine.GetProgress();
	CHECK(progress.isComplete);
	CHECK(progress.numFiles == 20);
	CHECK(progress.numFolders == 20);

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	CHECK(root->cur == 20);
	CHECK(root->SizeTotal() == 20 * 50);
	delete root;

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_async_scan_repeated_shutdown_stress()
{
	// Regression test for the shutdown lost-wakeup: run many quick scans
	// back to back; before the fix, a worker could miss the stop notification
	// and WaitForCompletion's join would hang.
	std::wstring tempDir = CreateTempTestDirectory();
	for (int i = 0; i < 4; ++i) {
		std::wstring sub = tempDir + L"\\sub" + std::to_wstring(i);
		CreateDirectoryW(sub.c_str(), NULL);
		CreateDummyFile(sub + L"\\file.txt", 10);
	}

	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	for (int i = 0; i < 100; ++i) {
		AsyncScanEngine engine;
		CHECK(engine.StartScan(preparedPath, 0, false, 4));
		engine.WaitForCompletion();
		CHECK(!engine.IsCancelled());
		CHECK(engine.IsFinished());

		CStringArena targetArena;
		CFolder* root = engine.DetachResult(targetArena);
		CHECK(root != NULL);
		CHECK(root->cur == 4);
		delete root;
	}

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_async_scan_is_scanning_loop_termination()
{
	std::wstring tempDir = CreateTempTestDirectory();
	for (int i = 0; i < 10; ++i) {
		std::wstring sub = tempDir + L"\\sub" + std::to_wstring(i);
		CreateDirectoryW(sub.c_str(), NULL);
		CreateDummyFile(sub + L"\\file.txt", 100);
	}

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 4095, true, 2));

	int loopIterations = 0;
	while (engine.IsScanning()) {
		Sleep(5);
		loopIterations++;
		CHECK(loopIterations < 500); // Must terminate within reasonable time (2.5s)
	}

	engine.WaitForCompletion();
	CHECK(!engine.IsCancelled());

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	delete root;

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_concurrent_live_layout()
{
	std::wstring tempDir = CreateTempTestDirectory();
	for (int i = 0; i < 40; ++i) {
		std::wstring sub = tempDir + L"\\dir" + std::to_wstring(i);
		CreateDirectoryW(sub.c_str(), NULL);
		for (int j = 0; j < 5; ++j) {
			CreateDummyFile(sub + L"\\file" + std::to_wstring(j) + L".txt", 100 + i * 10);
		}
	}

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 0, false, 4));

	TreemapConfig config;
	std::vector<TreemapNode> nodes;
	int liveLayoutCalls = 0;

	while (engine.IsScanning()) {
		engine.GenerateLiveLayout(800, 600, 5000000, 1000000, config, nodes);
		liveLayoutCalls++;
		if (!nodes.empty()) {
			for (const auto& n : nodes) {
				CHECK(n.w > 0 && n.h > 0);
				CHECK(n.x >= 0 && n.y >= 0);
			}
		}
		Sleep(2);
	}

	engine.WaitForCompletion();
	CHECK(!engine.IsCancelled());
	CHECK(liveLayoutCalls > 0);

	// Final live layout call after scan completion
	engine.GenerateLiveLayout(800, 600, 5000000, 1000000, config, nodes);
	CHECK(!nodes.empty());

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	delete root;

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_live_layout_nested_folders()
{
	std::wstring tempDir = CreateTempTestDirectory();

	// Structure:
	// tempDir/
	//   rootfile.txt (20,000 bytes)
	//   FolderA/
	//     FolderB/
	//       deepfile.txt (50,000 bytes)
	//   FolderC/
	//     file2.txt (30,000 bytes)

	CreateDummyFile(tempDir + L"\\rootfile.txt", 20000);

	std::wstring folderA = tempDir + L"\\FolderA";
	CreateDirectoryW(folderA.c_str(), NULL);
	std::wstring folderB = folderA + L"\\FolderB";
	CreateDirectoryW(folderB.c_str(), NULL);
	CreateDummyFile(folderB + L"\\deepfile.txt", 50000);

	std::wstring folderC = tempDir + L"\\FolderC";
	CreateDirectoryW(folderC.c_str(), NULL);
	CreateDummyFile(folderC + L"\\file2.txt", 30000);

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 0, false, 2));
	engine.WaitForCompletion();
	CHECK(!engine.IsCancelled());

	TreemapConfig config;
	std::vector<TreemapNode> nodes;
	engine.GenerateLiveLayout(1000, 800, 200000, 50000, config, nodes);

	CHECK(!nodes.empty());

	bool foundFolderA = false;
	bool foundFolderB = false;
	bool foundDeepFile = false;
	bool foundFolderC = false;
	bool foundRootFile = false;
	bool foundScanning = false;
	bool foundFreeSpace = false;

	for (const auto& n : nodes) {
		CHECK(n.name != nullptr);
		if (wcscmp(n.name, L"FolderA") == 0) {
			foundFolderA = true;
			CHECK(n.IsFolder());
			CHECK(n.depth == 0);
		}
		if (wcscmp(n.name, L"FolderB") == 0) {
			foundFolderB = true;
			CHECK(n.IsFolder());
			CHECK(n.depth == 1);
		}
		if (wcscmp(n.name, L"deepfile.txt") == 0) {
			foundDeepFile = true;
			CHECK(!n.IsFolder());
			CHECK(n.depth == 2);
		}
		if (wcscmp(n.name, L"FolderC") == 0) {
			foundFolderC = true;
			CHECK(n.IsFolder());
		}
		if (wcscmp(n.name, L"rootfile.txt") == 0) {
			foundRootFile = true;
			CHECK(!n.IsFolder());
		}
		if (wcscmp(n.name, L"<Scanning...>") == 0) {
			foundScanning = true;
		}
		if (wcscmp(n.name, L"<Free Space>") == 0) {
			foundFreeSpace = true;
			CHECK(n.IsSpecial());
		}
	}

	CHECK(foundFolderA);
	CHECK(foundFolderB);
	CHECK(foundDeepFile);
	CHECK(foundFolderC);
	CHECK(foundRootFile);
	CHECK(!foundScanning); // Scanning block is removed
	CHECK(foundFreeSpace);

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	delete root;

	DeleteTestDirectory(tempDir);
	return 1;
}

static int test_live_layout_during_active_multilevel_scan()
{
	std::wstring tempDir = CreateTempTestDirectory();

	// Create 30 top-level directories, each with a subfolder and files
	for (int i = 0; i < 30; ++i) {
		std::wstring sub1 = tempDir + L"\\TopDir" + std::to_wstring(i);
		CreateDirectoryW(sub1.c_str(), NULL);
		CreateDummyFile(sub1 + L"\\rootfile.txt", 1000);

		std::wstring sub2 = sub1 + L"\\SubDir" + std::to_wstring(i);
		CreateDirectoryW(sub2.c_str(), NULL);
		for (int j = 0; j < 3; ++j) {
			CreateDummyFile(sub2 + L"\\file" + std::to_wstring(j) + L".txt", 2000);
		}
	}

	AsyncScanEngine engine;
	std::wstring preparedPath = PathUtil::PrepareLongPath(tempDir);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	CHECK(engine.StartScan(preparedPath, 0, false, 4));

	TreemapConfig config;
	std::vector<TreemapNode> nodes;
	bool observedNestedFolderDuringScan = false;
	bool observedChildFileDuringScan = false;

	while (engine.IsScanning()) {
		engine.GenerateLiveLayout(1024, 768, 1000000, 200000, config, nodes);
		for (const auto& n : nodes) {
			if (n.IsFolder() && n.depth >= 1) {
				observedNestedFolderDuringScan = true;
			}
			if (!n.IsFolder() && n.depth >= 1) {
				observedChildFileDuringScan = true;
			}
		}
		Sleep(5);
	}

	engine.WaitForCompletion();
	CHECK(!engine.IsCancelled());

	// Must have observed nested folders and files during or immediately upon scan
	engine.GenerateLiveLayout(1024, 768, 1000000, 200000, config, nodes);
	for (const auto& n : nodes) {
		if (n.IsFolder() && n.depth >= 1) observedNestedFolderDuringScan = true;
		if (!n.IsFolder() && n.depth >= 1) observedChildFileDuringScan = true;
	}
	CHECK(observedNestedFolderDuringScan);
	CHECK(observedChildFileDuringScan);

	CStringArena targetArena;
	CFolder* root = engine.DetachResult(targetArena);
	CHECK(root != NULL);
	delete root;

	DeleteTestDirectory(tempDir);
	return 1;
}

int main()
{
	if (!test_async_scan_directory_tree()) return 1;
	if (!test_async_scan_cancellation()) return 1;
	if (!test_async_scan_rescan_after_cancel()) return 1;
	if (!test_async_scan_repeated_shutdown_stress()) return 1;
	if (!test_async_scan_is_scanning_loop_termination()) return 1;
	if (!test_concurrent_live_layout()) return 1;
	if (!test_live_layout_nested_folders()) return 1;
	if (!test_live_layout_during_active_multilevel_scan()) return 1;
	printf("AsyncScan_test passed successfully.\n");
	return 0;
}
