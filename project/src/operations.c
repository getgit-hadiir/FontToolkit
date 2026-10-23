/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : operations.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Font output operations, reports, and undo support.
*----------------------------------------------------------------*/
#include "app.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

int copy_file(const char *source, const char *destination) {
    FILE *in = fopen(source, "rb");
    FILE *out;
    unsigned char buffer[65536];
    size_t n;
    if (!in) return 0;
    out = fopen(destination, "wb");
    if (!out) { fclose(in); return 0; }
    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) {
            fclose(in); fclose(out); return 0;
        }
    }
    if (ferror(in)) { fclose(in); fclose(out); return 0; }
    fclose(in);
    if (fclose(out) != 0) return 0;
    return 1;
}

void log_message(const char *log_path, const char *message) {
    FILE *file = fopen(log_path, "a");
    if (!file) return;
    fprintf(file, "- %s\n", message);
    fclose(file);
}

int create_operation_dirs(const AppConfig *config, const char *operation,
                         char *operation_root, size_t root_size,
                         char *db_path, size_t db_size,
                         char *log_path, size_t log_size) {
    char output_root[MAX_PATH_LEN], stamp[32], data_dir[MAX_PATH_LEN], log_dir[MAX_PATH_LEN];
    if (!path_join(output_root, sizeof(output_root), config->workspace, "Output")) return 0;
    if (!ensure_directory(output_root)) return 0;
    timestamp_compact(stamp, sizeof(stamp));
    if (!path_join(operation_root, root_size, output_root, stamp)) return 0;
    if (path_exists(operation_root)) {
        int suffix = 1;
        do {
            char name[64];
            snprintf(name, sizeof(name), "%s_%d", stamp, suffix++);
            if (!path_join(operation_root, root_size, output_root, name)) return 0;
        } while (path_exists(operation_root));
    }
    if (!ensure_directory(operation_root)) return 0;
    if (!path_join(data_dir, sizeof(data_dir), operation_root, "._data") ||
        !path_join(log_dir, sizeof(log_dir), operation_root, "_log") ||
        !ensure_directory(data_dir) || !ensure_directory(log_dir) ||
        !path_join(db_path, db_size, data_dir, "appdata.db") ||
        !path_join(log_path, log_size, log_dir, "log.md")) return 0;
    {
        FILE *f = fopen(log_path, "w");
        if (!f) return 0;
        fprintf(f, "# FontToolkit operation log\n\n- Operation: %s\n", operation);
        fclose(f);
    }
    return 1;
}


int write_operation_report(const char *operation_root, const char *operation,
                           const FontList *list, size_t succeeded, size_t failed) {
    char report_path[MAX_PATH_LEN], name[128];
    const char *base = strrchr(operation_root, '/');
    FILE *file;
    size_t i, j, metadata_ok = 0, unknown = 0, long_names = 0, duplicate_names = 0, unique_families = 0;
    if (!base) base = strrchr(operation_root, '\\');
    base = base ? base + 1 : operation_root;
    snprintf(name, sizeof(name), "%s.md", base);
    if (!path_join(report_path, sizeof(report_path), operation_root, name)) return 0;
    file = fopen(report_path, "w");
    if (!file) return 0;
    for (i = 0; i < list->count; ++i) {
        if (list->items[i].metadata_valid) metadata_ok++;
        if (!list->items[i].family[0] || strcmp(list->items[i].family, "Unknown") == 0) unknown++;
        if (strlen(list->items[i].filename) > MAX_OUTPUT_NAME) long_names++;
        for (j = 0; j < i; ++j) {
            if (strcmp(list->items[i].filename, list->items[j].filename) == 0) {
                duplicate_names++;
                break;
            }
        }
        if (list->items[i].family[0]) {
            int seen = 0;
            for (j = 0; j < i; ++j) {
                if (strcmp(list->items[i].family, list->items[j].family) == 0) { seen = 1; break; }
            }
            if (!seen) unique_families++;
        }
    }
    fprintf(file, "# FontToolkit operation report\n\n");
    fprintf(file, "- Operation: %s\n- Created: %s\n- Files discovered: %zu\n", operation, base, list->count);
    fprintf(file, "- Metadata extracted: %zu\n- Metadata unavailable/unsupported: %zu\n", metadata_ok, list->count - metadata_ok);
    fprintf(file, "- Unknown family: %zu\n- Distinct family labels: %zu\n", unknown, unique_families);
    fprintf(file, "- Repeated source basenames: %zu\n- Source filenames over %d characters: %zu\n", duplicate_names, MAX_OUTPUT_NAME, long_names);
    fprintf(file, "- Files copied successfully: %zu\n- Failed operations: %zu\n\n", succeeded, failed);
    fprintf(file, "## File inventory\n\n| # | Source filename | Family | Subfamily | Metadata | Size (bytes) |\n|---:|---|---|---|---|---:|\n");
    for (i = 0; i < list->count; ++i) {
        const FontRecord *f = &list->items[i];
        char filename[600], family[600], subfamily[600];
        snprintf(filename, sizeof(filename), "%s", f->filename);
        snprintf(family, sizeof(family), "%s", f->family);
        snprintf(subfamily, sizeof(subfamily), "%s", f->subfamily);
        for (char *q = filename; *q; ++q) if (*q == '|') *q = '/';
        for (char *q = family; *q; ++q) if (*q == '|') *q = '/';
        for (char *q = subfamily; *q; ++q) if (*q == '|') *q = '/';
        fprintf(file, "| %zu | %s | %s | %s | %s | %lld |\n", i + 1, filename, family,
                subfamily, f->metadata_valid ? "OK" : "Partial/Unavailable", f->file_size);
    }
    fclose(file);
    return 1;
}

static int get_source_path(const AppConfig *config, char *out, size_t size) {
    return path_join(out, size, config->workspace, "Source");
}

static int safe_component(char *text) {
    size_t i, n = strlen(text);
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') text[i] = '_';
        else if (c < 32) text[i] = '_';
    }
    while (text[0] == '.' || text[0] == ' ') memmove(text, text + 1, strlen(text));
    while (n && (text[n-1] == '.' || text[n-1] == ' ')) text[--n] = '\0';
    if (!text[0]) snprintf(text, 2, "_");
    return 1;
}

static void remove_extension(const char *filename, char *out, size_t size) {
    const char *dot = strrchr(filename, '.');
    size_t n = dot ? (size_t)(dot - filename) : strlen(filename);
    if (n >= size) n = size - 1;
    memcpy(out, filename, n); out[n] = '\0';
}

static int unique_destination(const char *directory, const char *name,
                              char *out, size_t size) {
    char candidate[MAX_PATH_LEN];
    int suffix = 1;
    if (!path_join(candidate, sizeof(candidate), directory, name)) return 0;
    if (!path_exists(candidate)) { snprintf(out, size, "%s", candidate); return 1; }
    {
        char stem[512], ext[64];
        const char *dot = strrchr(name, '.');
        size_t n = dot ? (size_t)(dot-name) : strlen(name);
        if (n >= sizeof(stem)) n = sizeof(stem)-1;
        memcpy(stem, name, n); stem[n] = '\0';
        snprintf(ext, sizeof(ext), "%s", dot ? dot : "");
        do {
            char revised[640];
            snprintf(revised, sizeof(revised), "%s_%d%s", stem, suffix++, ext);
            if (!path_join(candidate, sizeof(candidate), directory, revised)) return 0;
        } while (path_exists(candidate));
    }
    snprintf(out, size, "%s", candidate);
    return 1;
}

static int scan_and_prepare(const AppConfig *config, FontList *list) {
    char source[MAX_PATH_LEN];
    if (!get_source_path(config, source, sizeof(source))) return 0;
    ui_print_wrappedf("Scanning source folder: %s", source);
    if (!scan_fonts(source, list, 1)) {
        ui_error_wrappedf("Unable to scan the source folder.");
        return 0;
    }
    ui_print_wrappedf("Deep scan complete. Font files: %zu", list->count);
    return 1;
}

int run_scan(const AppConfig *config) {
    FontList list;
    char root[MAX_PATH_LEN], db_path[MAX_PATH_LEN], log_path[MAX_PATH_LEN], message[1024];
    sqlite3 *db = NULL;
    if (!scan_and_prepare(config, &list)) return 0;
    if (!create_operation_dirs(config, "scan", root, sizeof(root), db_path, sizeof(db_path),
                               log_path, sizeof(log_path))) {
        font_list_free(&list); ui_error_wrappedf("Unable to create output operation folder."); return 0;
    }
    if (!database_initialize(db_path, &db)) { font_list_free(&list); return 0; }
    if (!database_store_fonts(db, &list)) ui_error_wrappedf("Warning: failed to save font metadata.");
    { size_t valid = 0; for (size_t j = 0; j < list.count; ++j) if (list.items[j].metadata_valid) valid++;
      snprintf(message, sizeof(message), "Scanned %zu font file(s); metadata extracted for %zu.", list.count, valid); }
    log_message(log_path, message);
    if (!write_operation_report(root, "scan", &list, 0, 0))
        ui_error_wrappedf("Warning: could not write Markdown report.");
    ui_print_wrapped("Scan report");
    ui_print_line('-');
    ui_print_wrappedf("Font files found: %zu", list.count);
    ui_print_wrappedf("Operation log: %s", log_path);
    ui_print_wrappedf("Database: %s", db_path);
    sqlite3_close(db);
    font_list_free(&list);
    return 1;
}

int run_rename(const AppConfig *config) {
    FontList list;
    char root[MAX_PATH_LEN], db_path[MAX_PATH_LEN], log_path[MAX_PATH_LEN], source_root[MAX_PATH_LEN];
    sqlite3 *db = NULL;
    size_t i, success = 0, failed = 0;
    if (!scan_and_prepare(config, &list)) return 0;
    if (!create_operation_dirs(config, "rename to original name", root, sizeof(root), db_path,
                               sizeof(db_path), log_path, sizeof(log_path))) {
        font_list_free(&list); return 0;
    }
    if (!database_initialize(db_path, &db)) { font_list_free(&list); return 0; }
    database_store_fonts(db, &list);
    path_join(source_root, sizeof(source_root), config->workspace, "Source");
    for (i = 0; i < list.count; ++i) {
        FontRecord *f = &list.items[i];
        char name[1024], dest[MAX_PATH_LEN], msg[1400];
        const char *ext = strrchr(f->filename, '.');
        const char *original = f->full_name[0] ? f->full_name :
                               (f->postscript_name[0] ? f->postscript_name : f->filename);
        snprintf(name, sizeof(name), "%s%s", original, ext ? ext : "");
        safe_component(name);
        if (strlen(name) > MAX_OUTPUT_NAME) {
            char long_dir[MAX_PATH_LEN], stem[512], shortname[128], sidecar[MAX_PATH_LEN];
            remove_extension(name, stem, sizeof(stem));
            snprintf(shortname, sizeof(shortname), "font_%04zu%s", i + 1, ext ? ext : "");
            path_join(long_dir, sizeof(long_dir), root, "Long_Names");
            ensure_directory(long_dir);
            if (!unique_destination(long_dir, shortname, dest, sizeof(dest)) ||
                !copy_file(f->path, dest)) { failed++; continue; }
            {
                char txtname[160];
                snprintf(txtname, sizeof(txtname), "font_%04zu.txt", i + 1);
                path_join(sidecar, sizeof(sidecar), long_dir, txtname);
                {
                    FILE *tf = fopen(sidecar, "w");
                    if (tf) { fprintf(tf, "Intended filename: %s\nFont family: %s\nSubfamily: %s\nSource path: %s\n", name, f->family, f->subfamily, f->path); fclose(tf); }
                }
            }
        } else {
            if (!unique_destination(root, name, dest, sizeof(dest)) ||
                !copy_file(f->path, dest)) { failed++; continue; }
        }
        database_store_action(db, f->path, dest, "rename-copy", f->filename, name);
        snprintf(msg, sizeof(msg), "Copied `%.600s` as `%.600s`.", f->path, dest);
        log_message(log_path, msg);
        success++;
    }
    if (!write_operation_report(root, "rename to original name", &list, success, failed)) ui_error_wrappedf("Warning: could not write Markdown report.");
    ui_print_wrapped("Rename operation complete.");
    ui_print_wrappedf("Copied: %zu", success);
    ui_print_wrappedf("Failed: %zu", failed);
    ui_print_wrappedf("Output: %s", root);
    sqlite3_close(db); font_list_free(&list);
    return failed == 0;
}

int run_group(const AppConfig *config) {
    FontList list;
    char root[MAX_PATH_LEN], db_path[MAX_PATH_LEN], log_path[MAX_PATH_LEN];
    sqlite3 *db = NULL;
    size_t i, success = 0, failed = 0;
    if (!scan_and_prepare(config, &list)) return 0;
    if (!create_operation_dirs(config, "group by font family", root, sizeof(root), db_path,
                               sizeof(db_path), log_path, sizeof(log_path))) {
        font_list_free(&list); return 0;
    }
    if (!database_initialize(db_path, &db)) { font_list_free(&list); return 0; }
    database_store_fonts(db, &list);
    for (i = 0; i < list.count; ++i) {
        FontRecord *f = &list.items[i];
        char family[600], family_dir[MAX_PATH_LEN], dest[MAX_PATH_LEN], msg[1400];
        snprintf(family, sizeof(family), "%s", f->family[0] ? f->family : "Unknown");
        safe_component(family);
        if (!path_join(family_dir, sizeof(family_dir), root, family) ||
            !ensure_directory(family_dir) ||
            !unique_destination(family_dir, f->filename, dest, sizeof(dest)) ||
            !copy_file(f->path, dest)) { failed++; continue; }
        database_store_action(db, f->path, dest, "group-copy", f->filename, f->filename);
        snprintf(msg, sizeof(msg), "Grouped `%s` into family `%s`.", f->filename, family);
        log_message(log_path, msg);
        success++;
    }
    if (!write_operation_report(root, "group by font family", &list, success, failed)) ui_error_wrappedf("Warning: could not write Markdown report.");
    ui_print_wrapped("Grouping operation complete.");
    ui_print_wrappedf("Copied: %zu", success);
    ui_print_wrappedf("Failed: %zu", failed);
    ui_print_wrappedf("Output: %s", root);
    sqlite3_close(db); font_list_free(&list);
    return failed == 0;
}


int run_undo(const AppConfig *config) {
    char output_root[MAX_PATH_LEN], folder_name[256], operation_root[MAX_PATH_LEN];
    char db_path[MAX_PATH_LEN], log_path[MAX_PATH_LEN], prefix[MAX_PATH_LEN];
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    size_t removed = 0, skipped = 0;
    int rc;
    ui_print_wrapped("Undo removes generated copies only; original Source files are never touched.");
    ui_print_wrapped("Enter operation folder name from Output:");
    ui_print_inlinef("Folder: ");
    if (!fgets(folder_name, sizeof(folder_name), stdin)) return 0;
    folder_name[strcspn(folder_name, "\r\n")] = '\0';
    while (folder_name[0] == ' ' || folder_name[0] == '\t')
        memmove(folder_name, folder_name + 1, strlen(folder_name));
    if (!folder_name[0] || strstr(folder_name, "..") || strchr(folder_name, '/') || strchr(folder_name, '\\')) {
        ui_error_wrappedf("Invalid operation folder name. Enter the folder name only.");
        return 0;
    }
    if (!path_join(output_root, sizeof(output_root), config->workspace, "Output") ||
        !path_join(operation_root, sizeof(operation_root), output_root, folder_name) ||
        !path_exists(operation_root) ||
        !path_join(prefix, sizeof(prefix), operation_root, "._data") ||
        !path_join(db_path, sizeof(db_path), prefix, "appdata.db") ||
        !path_join(prefix, sizeof(prefix), operation_root, "_log") ||
        !path_join(log_path, sizeof(log_path), prefix, "log.md")) {
        ui_error_wrappedf("Operation folder or database was not found.");
        return 0;
    }
    if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) {
        ui_error_wrappedf("Cannot open operation database: %s", db ? sqlite3_errmsg(db) : "unknown error");
        if (db) sqlite3_close(db);
        return 0;
    }
    if (sqlite3_prepare_v2(db, "SELECT destination_path FROM actions WHERE action IN ('rename-copy','group-copy') AND destination_path IS NOT NULL AND destination_path <> '' ORDER BY id DESC", -1, &stmt, NULL) != SQLITE_OK) {
        ui_error_wrappedf("No compatible action history: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *value = sqlite3_column_text(stmt, 0);
        char destination[MAX_PATH_LEN];
        size_t root_len = strlen(operation_root);
        if (!value) { skipped++; continue; }
        snprintf(destination, sizeof(destination), "%s", (const char *)value);
        path_normalize(destination);
        path_normalize(operation_root);
        if (strncmp(destination, operation_root, root_len) != 0 ||
            (destination[root_len] != '/' && destination[root_len] != '\0')) {
            ui_error_wrappedf("Skipped path outside operation folder: %s", destination);
            skipped++;
            continue;
        }
        if (remove(destination) == 0 || !path_exists(destination)) {
            removed++;
            {
                char message[1200];
                snprintf(message, sizeof(message), "Undo removed generated copy: %.1000s", destination);
                log_message(log_path, message);
            }
        } else {
            ui_error_wrappedf("Could not remove generated copy: %s", destination);
            skipped++;
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    ui_print_wrappedf("Undo complete. Removed: %zu; skipped/failed: %zu.", removed, skipped);
    ui_print_wrapped("Database, report, and log were retained.");
    return skipped == 0;
}

/* === End of file: operations.c === */
