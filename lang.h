
#ifndef LANG_H
#define LANG_H

struct Lang {
	int toolbar_bitmap_id;
	const wchar_t *about_spacemonger;
	const wchar_t *freeware;
	const wchar_t *warranty;
	const wchar_t *description;
	const wchar_t *maintainer;
	const wchar_t *project_website;
	const wchar_t *report_bug;
	const wchar_t *open_url_failed_format;
	const wchar_t *ok, *cancel;
	const wchar_t *bytes, *kb, *mb, *gb;
	const wchar_t *freespace_format;
	const wchar_t *zoomin, *zoomout, *zoomfull, *run, *del;
	const wchar_t *opendrive, *rescandrive, *showfreespace;
	const wchar_t *percent_format, *size_format;
	const wchar_t *total, *free, *files_total, *folders_total, *files_found, *folders_found;
	char digitpt, digitcomma;
	const wchar_t **densitynames;
	const wchar_t **colornames;
	const wchar_t **monthnames;
	const wchar_t **attribnames;
	const wchar_t *deleting;
	const wchar_t *selectdrive;
	const wchar_t *scanning;
	const wchar_t *settings;
	const wchar_t *layout, *density, *bias, *horz, *equal, *vert;
	const wchar_t *displaycolors, *files, *folders;
	const wchar_t *tooltips, *shownametips, *showinfotips, *delay, *msec;
	const wchar_t *fullpath, *filename, *smallicon, *icon, *datetime, *filesize, *attrib;
	const wchar_t *miscoptions;
	const wchar_t *autorescan, *disabledelete, *animatedzoom, *savepos;
	const wchar_t *showrolloverbox;
	const wchar_t *properties;
	const wchar_t *scan_failed_format, *scan_partial_format, *scan_partial_notice_format;
	const wchar_t *scan_partial_button_format, *scan_details_title;
	const wchar_t *scan_details_summary_format, *scan_details_copy, *scan_unavailable;
};

struct LangEntry {
	const char *abbrev;
	const wchar_t *name;
	Lang *text;
};

extern Lang *CurLang;		// Current language

extern Lang LangUs;			// English (US)
extern Lang LangUk;			// English (UK/Aust.)
extern Lang LangFr;			// Français (Nationale)

extern LangEntry Langs[];	// List of languages

#endif
