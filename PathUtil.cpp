#include "PathUtil.h"
#include <windows.h>
#include <algorithm>

namespace PathUtil {

std::wstring AnsiToWide(const std::string& str) {
	if (str.empty()) return L"";
	int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string WideToAnsi(const std::wstring& wstr) {
	if (wstr.empty()) return "";
	int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::string NormalizeSeparators(const std::string& path) {
	std::string normalized = path;
	std::replace(normalized.begin(), normalized.end(), '/', '\\');
	return normalized;
}

std::wstring NormalizeSeparators(const std::wstring& path) {
	std::wstring normalized = path;
	std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
	return normalized;
}

std::string Join(const std::string& base, const std::string& extra) {
	if (base.empty()) return NormalizeSeparators(extra);
	if (extra.empty()) return NormalizeSeparators(base);

	std::string normalizedBase = NormalizeSeparators(base);
	std::string normalizedExtra = NormalizeSeparators(extra);

	bool baseHasTrailing = (!normalizedBase.empty() && normalizedBase.back() == '\\');
	bool extraHasLeading = (!normalizedExtra.empty() && normalizedExtra.front() == '\\');

	if (baseHasTrailing && extraHasLeading) {
		return normalizedBase + normalizedExtra.substr(1);
	} else if (!baseHasTrailing && !extraHasLeading) {
		return normalizedBase + "\\" + normalizedExtra;
	} else {
		return normalizedBase + normalizedExtra;
	}
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

std::string EnsureTrailingBackslash(const std::string& path) {
	if (path.empty()) return "\\";
	std::string normalized = NormalizeSeparators(path);
	if (normalized.back() != '\\') {
		normalized += '\\';
	}
	return normalized;
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

std::wstring GetAbsolutePath(const std::string& path) {
	return GetAbsolutePath(AnsiToWide(path));
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

std::wstring PrepareLongPath(const std::string& path) {
	return PrepareLongPath(AnsiToWide(path));
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

std::wstring BuildWidePath(const char *root, const std::wstring& relativePath, const wchar_t *leafName) {
	std::wstring path = AnsiToWide(root == NULL ? "" : root);
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
