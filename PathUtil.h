#ifndef PATHUTIL_H
#define PATHUTIL_H

#ifdef bool
#pragma push_macro("bool")
#undef bool
#include <string>
#pragma pop_macro("bool")
#else
#include <string>
#endif

namespace PathUtil {

// Helper to convert ANSI/MBCS string to Wide string using CP_ACP
std::wstring AnsiToWide(const std::string& str);

// Helper to convert Wide string to ANSI/MBCS string using CP_ACP
std::string WideToAnsi(const std::wstring& wstr);

// Normalizes path separators to backslashes
std::string NormalizeSeparators(const std::string& path);
std::wstring NormalizeSeparators(const std::wstring& path);

// Combines two path components, ensuring exactly one backslash separator
std::string Join(const std::string& base, const std::string& extra);
std::wstring Join(const std::wstring& base, const std::wstring& extra);

// Ensures the path ends with a trailing backslash
std::string EnsureTrailingBackslash(const std::string& path);
std::wstring EnsureTrailingBackslash(const std::wstring& path);

// Resolves a relative path to a full absolute path using GetFullPathNameW
std::wstring GetAbsolutePath(const std::wstring& path);
std::wstring GetAbsolutePath(const std::string& path);

// Converts a path into a long-path-safe wide string (prefixed with \\?\ if appropriate)
std::wstring PrepareLongPath(const std::wstring& path);
std::wstring PrepareLongPath(const std::string& path);

} // namespace PathUtil

#endif // PATHUTIL_H
