#include "../UnicodeFontFallback.h"
#include <stdio.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			fflush(stderr); \
			return 1; \
		} \
	} while (0)

int main()
{
	// Use a display DC, matching both the treemap's paint DC and the original
	// missing-glyph reproduction (a compatible memory DC can map differently).
	HDC dc = ::GetDC(NULL);
	CHECK(dc != NULL);
	HFONT classic = ::CreateFontW(-9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		VARIABLE_PITCH|FF_SWISS, L"Small Fonts");
	CHECK(classic != NULL);
	std::vector<HFONT> fallbacks = SM_CreateUnicodeFallbackFonts(-9);
	CHECK(!fallbacks.empty());

	const wchar_t ascii[] = L"Readme.txt";
	CHECK(SM_FontSupportsText(dc, classic, ascii, (int)(sizeof(ascii) / sizeof(ascii[0])) - 1));
	CHECK(SM_SelectFontForText(dc, classic, fallbacks, ascii,
		(int)(sizeof(ascii) / sizeof(ascii[0])) - 1) == classic);

	// Include Han, Cyrillic, and Greek in one filesystem-style name. Small
	// Fonts with ANSI_CHARSET lacks these glyphs on the affected Windows host,
	// so a Unicode face must cover the whole name before the renderer chooses it.
	const wchar_t unicodeName[] = L"mix-\x4E2D\x0416\x03A9.txt";
	const int unicodeLength = (int)(sizeof(unicodeName) / sizeof(unicodeName[0])) - 1;
	if (SM_FontSupportsText(dc, classic, unicodeName, unicodeLength)) {
		CHECK(SM_SelectFontForText(dc, classic, fallbacks, unicodeName,
			unicodeLength) == classic);
		puts("Small Fonts covers the sample on this system");
	}
	else {
		bool hasCompleteFallback = false;
		for (size_t i = 0; i < fallbacks.size(); ++i)
			hasCompleteFallback = hasCompleteFallback ||
				SM_FontSupportsText(dc, fallbacks[i], unicodeName, unicodeLength);
		HFONT chosen = SM_SelectFontForText(dc, classic, fallbacks, unicodeName, unicodeLength);
		CHECK(chosen != classic);
		if (hasCompleteFallback)
			CHECK(SM_FontSupportsText(dc, chosen, unicodeName, unicodeLength));
		else
			puts("No installed candidate covers the complete mixed-script sample; fallback remains best effort");
	}

	for (size_t i = 0; i < fallbacks.size(); ++i) ::DeleteObject(fallbacks[i]);
	::DeleteObject(classic);
	::ReleaseDC(NULL, dc);
	puts("Unicode font fallback selection passed");
	return 0;
}
