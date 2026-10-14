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

// Normalizes path separators to backslashes
std::wstring NormalizeSeparators(const std::wstring& path);

// Combines two path components, ensuring exactly one backslash separator
std::wstring Join(const std::wstring& base, const std::wstring& extra);

// Ensures the path ends with a trailing backslash
std::wstring EnsureTrailingBackslash(const std::wstring& path);

// Resolves a relative path to a full absolute path using GetFullPathNameW
std::wstring GetAbsolutePath(const std::wstring& path);

// Converts a path into a long-path-safe wide string (prefixed with \\?\ if appropriate)
std::wstring PrepareLongPath(const std::wstring& path);

// Converts long-path-prefixed paths back to ordinary Win32 shell paths.
std::wstring RemoveLongPathPrefix(const std::wstring& path);

// Appends one path component and returns the path length before appending.
std::wstring::size_type AppendComponent(std::wstring& path, const wchar_t *component);

// Builds a wide path from the app root and wide scanner path parts.
std::wstring BuildWidePath(const wchar_t *root, const std::wstring& relativePath, const wchar_t *leafName);

} // namespace PathUtil

#endif // PATHUTIL_H
