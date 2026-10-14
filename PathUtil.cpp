#include "PathUtil.h"
#include <windows.h>
#include <algorithm>

namespace PathUtil {

std::wstring NormalizeSeparators(const std::wstring& path) {
	std::wstring normalized = path;
	std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
	return normalized;
}

std::wstring Join(const std::wstring& base, const std::wstring& extra) {
	if (base.empty()) return NormalizeSeparators(extra);
	if (extra.empty()) return NormalizeSeparators(base);

	std::wstring normalizedBase = NormalizeSeparators(base);
	std::wstring normalizedExtra = NormalizeSeparators(extra);

	bool baseHasTrailing = (!normalizedBase.empty() && normalizedBase.back() == L'\\');
	bool extraHasLeading = (!normalizedExtra.empty() && normalizedExtra.front() == L'\\');

	if (baseHasTrailing && extraHasLeading) {
		return normalizedBase + normalizedExtra.substr(1);
	} else if (!baseHasTrailing && !extraHasLeading) {
		return normalizedBase + L"\\" + normalizedExtra;
	} else {
		return normalizedBase + normalizedExtra;
	}
}

std::wstring EnsureTrailingBackslash(const std::wstring& path) {
	if (path.empty()) return L"\\";
	std::wstring normalized = NormalizeSeparators(path);
	if (normalized.back() != L'\\') {
		normalized += L'\\';
	}
	return normalized;
}

std::wstring GetAbsolutePath(const std::wstring& path) {
	if (path.empty()) return L"";

	// Get full path size requirements
	DWORD required = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
	if (required == 0) return path;

	std::wstring result(required, L'\0');
	DWORD written = GetFullPathNameW(path.c_str(), required, &result[0], NULL);
	if (written == 0 || written >= required) return path;

	result.resize(written);
	return result;
}

std::wstring PrepareLongPath(const std::wstring& path) {
	if (path.empty()) return L"";

	std::wstring normalized = NormalizeSeparators(path);

	// If it already has device or long path prefixes, return as is.
	if (normalized.rfind(L"\\\\?\\", 0) == 0 || normalized.rfind(L"\\\\.\\", 0) == 0) {
		return normalized;
	}

	// Resolve to absolute path
	std::wstring absolute = GetAbsolutePath(normalized);

	// Check again in case it got resolved to a device path (unlikely but safe)
	if (absolute.rfind(L"\\\\?\\", 0) == 0 || absolute.rfind(L"\\\\.\\", 0) == 0) {
		return absolute;
	}

	if (absolute.rfind(L"\\\\", 0) == 0) {
		// UNC path: \\server\share -> \\?\UNC\server\share
		return L"\\\\?\\UNC\\" + absolute.substr(2);
	} else if (absolute.size() >= 2 && absolute[1] == L':') {
		// Local path: C:\foo -> \\?\C:\foo
		return L"\\\\?\\" + absolute;
	}

	return absolute;
}

std::wstring RemoveLongPathPrefix(const std::wstring& path) {
	if (path.rfind(L"\\\\?\\UNC\\", 0) == 0) {
		return L"\\\\" + path.substr(8);
	}
	if (path.rfind(L"\\\\?\\", 0) == 0) {
		return path.substr(4);
	}
	return path;
}

std::wstring::size_type AppendComponent(std::wstring& path, const wchar_t *component) {
	std::wstring::size_type originalLength = path.size();
	if (component == NULL || component[0] == L'\0') {
		return originalLength;
	}

	if (!path.empty() && path.back() != L'\\') {
		path += L'\\';
	}
	path += component;
	return originalLength;
}

std::wstring BuildWidePath(const wchar_t *root, const std::wstring& relativePath, const wchar_t *leafName) {
	std::wstring path = root == NULL ? L"" : root;
	std::wstring relative = NormalizeSeparators(relativePath);

	path = NormalizeSeparators(path);
	if (!relative.empty()) {
		if (!path.empty() && path.back() != L'\\' && relative.front() != L'\\') {
			path += L'\\';
		} else if (!path.empty() && path.back() == L'\\' && relative.front() == L'\\') {
			relative.erase(0, 1);
		}
		path += relative;
	}

	AppendComponent(path, leafName);
	return path;
}

} // namespace PathUtil
