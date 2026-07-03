#include "../PathUtil.h"
#include <iostream>
#include <stdio.h>
#include <windows.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

int test_ansi_wide_conversion() {
	std::string ansi = "C:\\Windows\\System32";
	std::wstring wide = PathUtil::AnsiToWide(ansi);
	CHECK(wide == L"C:\\Windows\\System32");

	std::string ansi_back = PathUtil::WideToAnsi(wide);
	CHECK(ansi_back == ansi);
	return 1;
}

int test_normalize_separators() {
	CHECK(PathUtil::NormalizeSeparators("C:/foo/bar") == "C:\\foo\\bar");
	CHECK(PathUtil::NormalizeSeparators("C:\\foo/bar") == "C:\\foo\\bar");
	CHECK(PathUtil::NormalizeSeparators(L"C:/foo/bar") == L"C:\\foo\\bar");
	return 1;
}

int test_join() {
	CHECK(PathUtil::Join("C:\\foo", "bar") == "C:\\foo\\bar");
	CHECK(PathUtil::Join("C:\\foo\\", "bar") == "C:\\foo\\bar");
	CHECK(PathUtil::Join("C:\\foo", "\\bar") == "C:\\foo\\bar");
	CHECK(PathUtil::Join("C:\\foo\\", "\\bar") == "C:\\foo\\bar");
	CHECK(PathUtil::Join("", "bar") == "bar");
	CHECK(PathUtil::Join("C:\\foo", "") == "C:\\foo");

	CHECK(PathUtil::Join(L"C:\\foo", L"bar") == L"C:\\foo\\bar");
	CHECK(PathUtil::Join(L"C:\\foo\\", L"bar") == L"C:\\foo\\bar");
	CHECK(PathUtil::Join(L"C:\\foo", L"\\bar") == L"C:\\foo\\bar");
	return 1;
}

int test_ensure_trailing_backslash() {
	CHECK(PathUtil::EnsureTrailingBackslash("C:\\foo") == "C:\\foo\\");
	CHECK(PathUtil::EnsureTrailingBackslash("C:\\foo\\") == "C:\\foo\\");
	CHECK(PathUtil::EnsureTrailingBackslash("") == "\\");

	CHECK(PathUtil::EnsureTrailingBackslash(L"C:\\foo") == L"C:\\foo\\");
	return 1;
}

int test_get_absolute_path() {
	// Relative path "." should resolve to the current directory
	std::wstring current = PathUtil::GetAbsolutePath(L".");
	CHECK(!current.empty());
	CHECK(current.find(L"\\..") == std::wstring::npos);

	// Resolving dot-dot path
	std::wstring path_dot_dot = PathUtil::GetAbsolutePath(L"C:\\Windows\\System32\\..");
	CHECK(path_dot_dot == L"C:\\Windows");
	return 1;
}

int test_prepare_long_path() {
	// Already prefixed local path
	CHECK(PathUtil::PrepareLongPath(L"\\\\?\\C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");
	// Already prefixed device path
	CHECK(PathUtil::PrepareLongPath(L"\\\\.\\PhysicalDrive0") == L"\\\\.\\PhysicalDrive0");

	// Local path
	CHECK(PathUtil::PrepareLongPath(L"C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");
	CHECK(PathUtil::PrepareLongPath("C:\\foo\\bar") == L"\\\\?\\C:\\foo\\bar");

	// UNC path
	CHECK(PathUtil::PrepareLongPath(L"\\\\server\\share\\file") == L"\\\\?\\UNC\\server\\share\\file");
	CHECK(PathUtil::PrepareLongPath("\\\\server\\share\\file") == L"\\\\?\\UNC\\server\\share\\file");
	return 1;
}

int test_remove_long_path_prefix() {
	CHECK(PathUtil::RemoveLongPathPrefix(L"\\\\?\\C:\\foo\\bar") == L"C:\\foo\\bar");
	CHECK(PathUtil::RemoveLongPathPrefix(L"\\\\?\\UNC\\server\\share\\file") == L"\\\\server\\share\\file");
	CHECK(PathUtil::RemoveLongPathPrefix(L"C:\\foo\\bar") == L"C:\\foo\\bar");
	CHECK(PathUtil::RemoveLongPathPrefix(L"\\\\.\\PhysicalDrive0") == L"\\\\.\\PhysicalDrive0");
	return 1;
}

int test_append_component_returns_original_length() {
	std::wstring path = L"\\\\?\\C:\\root\\";
	size_t original = PathUtil::AppendComponent(path, L"child");

	CHECK(original == 12);
	CHECK(path == L"\\\\?\\C:\\root\\child");

	path.resize(original);
	CHECK(path == L"\\\\?\\C:\\root\\");
	return 1;
}

int test_append_component_adds_separator_when_needed() {
	std::wstring path = L"C:\\root";
	PathUtil::AppendComponent(path, L"child");

	CHECK(path == L"C:\\root\\child");
	return 1;
}

int test_build_wide_path_preserves_non_ascii_name() {
	std::wstring path = PathUtil::BuildWidePath("C:\\root", L"subfolder\\", L"f\u00F8.txt");

	CHECK(path == L"C:\\root\\subfolder\\f\u00F8.txt");
	return 1;
}

int test_prepare_built_wide_path_adds_long_path_prefix() {
	std::wstring path = PathUtil::BuildWidePath("C:\\root\\", L"", L"child.txt");
	std::wstring prepared = PathUtil::PrepareLongPath(path);

	CHECK(prepared == L"\\\\?\\C:\\root\\child.txt");
	return 1;
}

int main() {
	if (!test_ansi_wide_conversion()) return 1;
	if (!test_normalize_separators()) return 1;
	if (!test_join()) return 1;
	if (!test_ensure_trailing_backslash()) return 1;
	if (!test_get_absolute_path()) return 1;
	if (!test_prepare_long_path()) return 1;
	if (!test_remove_long_path_prefix()) return 1;
	if (!test_append_component_returns_original_length()) return 1;
	if (!test_append_component_adds_separator_when_needed()) return 1;
	if (!test_build_wide_path_preserves_non_ascii_name()) return 1;
	if (!test_prepare_built_wide_path_adds_long_path_prefix()) return 1;
	std::cout << "All PathUtil tests passed successfully!\n";
	return 0;
}
