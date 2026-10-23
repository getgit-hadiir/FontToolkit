/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : database.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : SQLite database initialization and persistence.
*----------------------------------------------------------------*/
#include "app.h"
#include <stdio.h>
#include <string.h>

int database_initialize(const char *db_path, sqlite3 **db) {
    const char *schema =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE IF NOT EXISTS fonts ("
        "id INTEGER PRIMARY KEY, source_path TEXT NOT NULL, filename TEXT NOT NULL,"
        "family TEXT, subfamily TEXT, full_name TEXT, postscript_name TEXT, file_size INTEGER, metadata_valid INTEGER DEFAULT 0,"
        "scanned_at TEXT DEFAULT CURRENT_TIMESTAMP);"
        "CREATE TABLE IF NOT EXISTS actions ("
        "id INTEGER PRIMARY KEY, action TEXT NOT NULL, source_path TEXT NOT NULL,"
        "destination_path TEXT, old_name TEXT, new_name TEXT, created_at TEXT DEFAULT CURRENT_TIMESTAMP);";
    if (sqlite3_open(db_path, db) != SQLITE_OK) {
        ui_error_wrappedf("SQLite error: %s", *db ? sqlite3_errmsg(*db) : "unable to open database");
        if (*db) sqlite3_close(*db);
        *db = NULL;
        return 0;
    }
    if (sqlite3_exec(*db, schema, NULL, NULL, NULL) != SQLITE_OK) {
        ui_error_wrappedf("SQLite schema error: %s", sqlite3_errmsg(*db));
        sqlite3_close(*db); *db = NULL; return 0;
    }
    return 1;
}

int database_store_fonts(sqlite3 *db, const FontList *list) {
    sqlite3_stmt *stmt = NULL;
    size_t i;
    const char *sql = "INSERT INTO fonts(source_path,filename,family,subfamily,full_name,postscript_name,file_size,metadata_valid) VALUES(?,?,?,?,?,?,?,?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    for (i = 0; i < list->count; ++i) {
        const FontRecord *f = &list->items[i];
        sqlite3_bind_text(stmt, 1, f->path, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, f->filename, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, f->family, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, f->subfamily, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, f->full_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, f->postscript_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 7, f->file_size);
        sqlite3_bind_int(stmt, 8, f->metadata_valid);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
            return 0;
        }
        sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
    return sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) == SQLITE_OK;
}

int database_store_action(sqlite3 *db, const char *source, const char *destination,
                          const char *action, const char *old_name, const char *new_name) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO actions(action,source_path,destination_path,old_name,new_name) VALUES(?,?,?,?,?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, action, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, source, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, destination ? destination : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, old_name ? old_name : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, new_name ? new_name : "", -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); return 0; }
    sqlite3_finalize(stmt);
    return 1;
}

/* === End of file: database.c === */
