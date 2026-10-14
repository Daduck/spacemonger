#ifndef UNICODE_FONT_FALLBACK_H
#define UNICODE_FONT_FALLBACK_H

#include <windows.h>
#include <string.h>
#include <vector>

inline std::vector<HFONT> SM_CreateUnicodeFallbackFonts(int height)
{
	static const wchar_t *families[] = {
		L"Segoe UI", L"Yu Gothic", L"Meiryo", L"MS Gothic", L"MingLiU",
		L"Microsoft YaHei UI", L"Microsoft YaHei", L"SimSun", L"Malgun Gothic",
		L"Gulim", L"Arial Unicode MS", L"Arial"
	};
	std::vector<HFONT> fonts;
	for (size_t i = 0; i < sizeof(families) / sizeof(families[0]); ++i) {
		HFONT font = ::CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, families[i]);
		if (font != NULL) fonts.push_back(font);
	}
	return fonts;
}

// GetGlyphIndicesW can report a missing glyph without drawing the text. Use
// this to keep the original Small Fonts appearance where it has coverage and
// choose an installed Unicode font for names containing unsupported scripts.
inline bool SM_FontSupportsText(HDC dc, HFONT font, const wchar_t *text, int length)
{
	if (dc == NULL || font == NULL || text == NULL || length < 0) return false;
	if (length == 0) return true;

	HGDIOBJ oldFont = ::SelectObject(dc, font);
	if (oldFont == NULL || oldFont == HGDI_ERROR) return false;

	bool supported = true;
	WORD glyphs[64];
	for (int offset = 0; offset < length && supported;) {
		int count = length - offset;
		if (count > (int)(sizeof(glyphs) / sizeof(glyphs[0])))
			count = (int)(sizeof(glyphs) / sizeof(glyphs[0]));
		memset(glyphs, 0xFF, sizeof(glyphs));
		DWORD result = ::GetGlyphIndicesW(dc, text + offset, count, glyphs,
			GGI_MARK_NONEXISTING_GLYPHS);
		if (result == GDI_ERROR || result != (DWORD)count) {
			supported = false;
			break;
		}
		for (int i = 0; i < count; ++i) {
			if (glyphs[i] == 0xFFFF) {
				supported = false;
				break;
			}
		}
		offset += count;
	}

	::SelectObject(dc, oldFont);
	return supported;
}

inline HFONT SM_SelectFontForText(HDC dc, HFONT classicFont,
	const std::vector<HFONT>& fallbackFonts, const wchar_t *text, int length)
{
	if (SM_FontSupportsText(dc, classicFont, text, length)) return classicFont;
	for (size_t i = 0; i < fallbackFonts.size(); ++i) {
		if (SM_FontSupportsText(dc, fallbackFonts[i], text, length))
			return fallbackFonts[i];
	}
	// Keep a Unicode font selected as the best available rendering attempt if
	// no installed face covers every character in a mixed-script name.
	return fallbackFonts.empty() ? classicFont : fallbackFonts[0];
}

#endif
