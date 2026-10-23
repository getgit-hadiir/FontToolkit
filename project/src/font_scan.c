/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : font_scan.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Recursive font discovery and metadata scan workflow.
*----------------------------------------------------------------*/
#include "app.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

static int has_font_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return (strcasecmp(dot, ".ttf") == 0 || strcasecmp(dot, ".otf") == 0);
}

static int list_add(FontList *list, const char *path) {
    struct stat st;
    FontRecord *item;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return 1;
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 64;
        FontRecord *next = (FontRecord *)realloc(list->items, new_capacity * sizeof(FontRecord));
        if (!next) return 0;
        list->items = next; list->capacity = new_capacity;
    }
    item = &list->items[list->count++];
    memset(item, 0, sizeof(*item));
    snprintf(item->path, sizeof(item->path), "%s", path);
    {
        const char *slash = strrchr(path, '/');
        const char *backslash = strrchr(path, '\\');
        const char *base = slash;
        if (!base || (backslash && backslash > base)) base = backslash;
        snprintf(item->filename, sizeof(item->filename), "%.*s", (int)sizeof(item->filename) - 1, base ? base + 1 : path);
    }
    item->file_size = (long long)st.st_size;
    snprintf(item->family, sizeof(item->family), "Unknown");
    snprintf(item->subfamily, sizeof(item->subfamily), "Regular");
    snprintf(item->full_name, sizeof(item->full_name), "%s", item->filename);
    return 1;
}

#ifndef _WIN32
static int scan_directory(const char *root, FontList *list, int show_progress) {
    DIR *dir = opendir(root);
    struct dirent *entry;
    if (!dir) return 1;
    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH_LEN];
        struct stat st;
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (!path_join(path, sizeof(path), root, entry->d_name)) continue;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (!scan_directory(path, list, show_progress)) { closedir(dir); return 0; }
        } else if (S_ISREG(st.st_mode) && has_font_extension(entry->d_name)) {
            if (!list_add(list, path)) { closedir(dir); return 0; }
            if (show_progress && list->count % 25 == 0) {
                ui_print_progressf("Scan: %zu fonts found...", list->count);
            }
        }
    }
    closedir(dir);
    return 1;
}
#else
static int scan_directory(const char *root, FontList *list, int show_progress) {
    char pattern[MAX_PATH_LEN];
    WIN32_FIND_DATAA data;
    HANDLE h;
    if (!path_join(pattern, sizeof(pattern), root, "*")) return 1;
    h = FindFirstFileA(pattern, &data);
    if (h == INVALID_HANDLE_VALUE) return 1;
    do {
        char path[MAX_PATH_LEN];
        if (!strcmp(data.cFileName, ".") || !strcmp(data.cFileName, "..")) continue;
        if (!path_join(path, sizeof(path), root, data.cFileName)) continue;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!scan_directory(path, list, show_progress)) { FindClose(h); return 0; }
        } else if (has_font_extension(data.cFileName)) {
            if (!list_add(list, path)) { FindClose(h); return 0; }
            if (show_progress && list->count % 25 == 0) {
                ui_print_progressf("Scan: %zu fonts found...", list->count);
            }
        }
    } while (FindNextFileA(h, &data));
    FindClose(h);
    return 1;
}
#endif

int scan_fonts(const char *root, FontList *list, int show_progress) {
    size_t i;
    memset(list, 0, sizeof(*list));
    if (!root || !path_exists(root)) return 0;
    if (!scan_directory(root, list, show_progress)) {
        font_list_free(list); return 0;
    }
    if (show_progress) {
        if (list->count >= 25) ui_print_progress_end();
        ui_print_wrappedf("Quick scan complete: %zu font files found.", list->count);
        ui_print_wrapped("Deep scan: extracting metadata...");
    }
    for (i = 0; i < list->count; ++i) {
        list->items[i].metadata_valid = read_font_metadata(&list->items[i]);
        if (show_progress && ((i + 1) % 25 == 0 || i + 1 == list->count)) {
            ui_print_progressf("Scan: %zu/%zu processed...", i + 1, list->count);
        }
    }
    if (show_progress) {
        if (list->count > 0) ui_print_progress_end();
        ui_print_wrappedf("Deep scan complete: %zu/%zu files.",
                          list->count, list->count);
    }
    return 1;
}

void font_list_free(FontList *list) {
    if (!list) return;
    free(list->items);
    list->items = NULL; list->count = 0; list->capacity = 0;
}

/* === End of file: font_scan.c === */
