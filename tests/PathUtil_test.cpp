#include "../PathUtil.h"
#include <assert.h>
#include <iostream>
#include <windows.h>

void test_ansi_wide_conversion() {
	std::string ansi = "C:\\Windows\\System32";
	std::wstring wide = PathUtil::AnsiToWide(ansi);
	assert(wide == L"C:\\Windows\\System32");

	std::string ansi_back = PathUtil::WideToAnsi(wide);
	assert(ansi_back == ansi);
}

void test_normalize_separators() {
	assert(PathUtil::NormalizeSeparators("C:/foo/bar") == "C:\\foo\\bar");
	assert(PathUtil::NormalizeSeparators("C:\\foo/bar") == "C:\\foo\\bar");
	assert(PathUtil::NormalizeSeparators(L"C:/foo/bar") == L"C:\\foo\\bar");
}

void test_join() {
	assert(PathUtil::Join("C:\\foo", "bar") == "C:\\foo\\bar");
	assert(PathUtil::Join("C:\\foo\\", "bar") == "C:\\foo\\bar");
	assert(PathUtil::Join("C:\\foo", "\\bar") == "C:\\foo\\bar");
	assert(PathUtil::Join("C:\\foo\\", "\\bar") == "C:\\foo\\bar");
	assert(PathUtil::Join("", "bar") == "bar");
	assert(PathUtil::Join("C:\\foo", "") == "C:\\foo");

	assert(PathUtil::Join(L"C:\\foo", L"bar") == L"C:\\foo\\bar");
	assert(PathUtil::Join(L"C:\\foo\\", L"bar") == L"C:\\foo\\bar");
	assert(PathUtil::Join(L"C:\\foo", L"\\bar") == L"C:\\foo\\bar");
}

void test_ensure_trailing_backslash() {
	assert(PathUtil::EnsureTrailingBackslash("C:\\foo") == "C:\\foo\\");
	assert(PathUtil::EnsureTrailingBackslash("C:\\foo\\") == "C:\\foo\\");
	assert(PathUtil::EnsureTrailingBackslash("") == "\\");

	assert(PathUtil::EnsureTrailingBackslash(L"C:\\foo") == L"C:\\foo\\");
}

void test_get_absolute_path() {
	// Relative path "." should resolve to the current directory
	std::wstring current = PathUtil::GetAbsolutePath(L".");
	assert(!current.empty());
	assert(current.find(L"\\..") == std::wstring::npos);

	// Resolving dot-dot path
	std::wstring path_dot_dot = PathUtil::GetAbsolutePath(L"C:\\Windows\\System32\\..");
	assert(path_dot_dot == L"C:\\Windows");
}

void test_prepare_long_path() {
	// Already prefixed local path
	assert(PathUtil::PrepareLongPath(L"\\\\?\\C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");
	// Already prefixed device path
	assert(PathUtil::PrepareLongPath(L"\\\\.\\PhysicalDrive0") == L"\\\\.\\PhysicalDrive0");

	// Local path
	assert(PathUtil::PrepareLongPath(L"C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");
	assert(PathUtil::PrepareLongPath("C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");

	// UNC path
	assert(PathUtil::PrepareLongPath(L"\\\\server\\share\\file") == L"\\\\?\\UNC\\server\\share\\file");
	assert(PathUtil::PrepareLongPath("\\\\server\\share\\file") == L"\\\\?\\UNC\\server\\share\\file");
}

int main() {
	test_ansi_wide_conversion();
	test_normalize_separators();
	test_join();
	test_ensure_trailing_backslash();
	test_get_absolute_path();
	test_prepare_long_path();
	std::cout << "All PathUtil tests passed successfully!\n";
	return 0;
}
