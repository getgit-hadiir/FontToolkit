/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : app.h
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Application-wide data structures and interfaces.
*----------------------------------------------------------------*/
#ifndef FONTTOOLKIT_APP_H
#define FONTTOOLKIT_APP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sqlite3.h>
#include "common.h"
#include "ui.h"
#include "uifx.h"

#define APP_NAME "FontToolkit"
#define APP_TITLE "Font Toolkit - Font Collection Organizer"
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 1
#define APP_DESCRIPTION "Scan, rename, and group TTF/OTF font files"
#define MAX_PATH_LEN COMMON_PATH_MAX
#define MAX_TEXT_LEN COMMON_TEXT_MAX
#define MAX_OUTPUT_NAME COMMON_OUTPUT_NAME_MAX

typedef struct {
    char project_name[64];
    char project_title[128];
    char description[129];
    unsigned long run_count;
    int version_major;
    int version_minor;
    int version_patch;
    char last_updated[32];
    char workspace[MAX_PATH_LEN];
} AppConfig;

typedef struct {
    char path[MAX_PATH_LEN];
    char filename[512];
    char family[512];
    char subfamily[512];
    char full_name[512];
    char postscript_name[512];
    long long file_size;
    int metadata_valid;
} FontRecord;

typedef struct {
    FontRecord *items;
    size_t count;
    size_t capacity;
} FontList;

int config_load_or_create(AppConfig *config);
int config_save(const AppConfig *config);
int ensure_directory(const char *path);
int path_join(char *out, size_t out_size, const char *left, const char *right);
int path_exists(const char *path);
void path_normalize(char *path);
void timestamp_compact(char *out, size_t out_size);
void timestamp_iso(char *out, size_t out_size);
int scan_fonts(const char *root, FontList *list, int show_progress);
void font_list_free(FontList *list);
int read_font_metadata(FontRecord *record);
int database_initialize(const char *db_path, sqlite3 **db);
int database_store_fonts(sqlite3 *db, const FontList *list);
int database_store_action(sqlite3 *db, const char *source, const char *destination,
                          const char *action, const char *old_name, const char *new_name);
int create_operation_dirs(const AppConfig *config, const char *operation,
                         char *operation_root, size_t root_size,
                         char *db_path, size_t db_size,
                         char *log_path, size_t log_size);
void log_message(const char *log_path, const char *message);
int write_operation_report(const char *operation_root, const char *operation,
                           const FontList *list, size_t succeeded, size_t failed);
int copy_file(const char *source, const char *destination);
int run_scan(const AppConfig *config);
int run_rename(const AppConfig *config);
int run_group(const AppConfig *config);
int run_undo(const AppConfig *config);
void print_banner(const AppConfig *config);
void print_main_menu(void);

#endif

/* === End of file: app.h === */
