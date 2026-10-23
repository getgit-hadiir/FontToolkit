/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : config.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Application configuration and workspace initialization.
*----------------------------------------------------------------*/
#include "app.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0775)
#endif

static void trim(char *text) {
    size_t n;
    char *start = text;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != text) memmove(text, start, strlen(start) + 1);
    n = strlen(text);
    while (n && isspace((unsigned char)text[n - 1])) text[--n] = '\0';
}

int path_exists(const char *path) {
#ifdef _WIN32
    return _access(path, 0) == 0;
#else
    return access(path, F_OK) == 0;
#endif
}

int ensure_directory(const char *path) {
    char buffer[MAX_PATH_LEN];
    size_t i, n;
    if (!path || !*path) return 0;
    snprintf(buffer, sizeof(buffer), "%s", path);
    path_normalize(buffer);
    n = strlen(buffer);
    for (i = 1; i < n; ++i) {
        if (buffer[i] == '/') {
            char saved = buffer[i];
            buffer[i] = '\0';
            if (buffer[0] && !path_exists(buffer) && MKDIR(buffer) != 0 && errno != EEXIST) {
                buffer[i] = saved; return 0;
            }
            buffer[i] = saved;
        }
    }
    if (!path_exists(buffer) && MKDIR(buffer) != 0 && errno != EEXIST) return 0;
    return 1;
}

int path_join(char *out, size_t out_size, const char *left, const char *right) {
    size_t len;
    if (!out || !left || !right || out_size == 0) return 0;
    len = strlen(left);
    {
        size_t right_len = strlen(right);
        int separator = (len && left[len - 1] != '/' && left[len - 1] != '\\');
        if (len + (size_t)separator + right_len + 1 > out_size) return 0;
        memcpy(out, left, len);
        if (separator) out[len++] = '/';
        memcpy(out + len, right, right_len + 1);
    }
    return 1;
}

void path_normalize(char *path) {
    size_t i;
    if (!path) return;
    for (i = 0; path[i]; ++i) {
        if (path[i] == '\\') path[i] = '/';
    }
}

void timestamp_compact(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (!tm_info) { snprintf(out, out_size, "unknown_time"); return; }
    strftime(out, out_size, "%Y%m%d_%H%M", tm_info);
}

void timestamp_iso(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (!tm_info) { snprintf(out, out_size, "unknown"); return; }
    strftime(out, out_size, "%Y-%m-%d %H:%M", tm_info);
}

static int make_config_path(char *out, size_t size) {
    char cwd[MAX_PATH_LEN];
#ifdef _WIN32
    if (!_getcwd(cwd, sizeof(cwd))) return 0;
#else
    if (!getcwd(cwd, sizeof(cwd))) return 0;
#endif
    return path_join(out, size, cwd, "user/project.conf");
}

int config_save(const AppConfig *config) {
    char path[MAX_PATH_LEN];
    FILE *file;
    if (!make_config_path(path, sizeof(path))) return 0;
    if (!ensure_directory("user")) return 0;
    file = fopen(path, "w");
    if (!file) return 0;
    fprintf(file, "project_name=%s\n", config->project_name);
    fprintf(file, "project_title=%s\n", config->project_title);
    fprintf(file, "description=%s\n", config->description);
    fprintf(file, "run_count=%lu\n", config->run_count);
    fprintf(file, "version_major=%d\n", config->version_major);
    fprintf(file, "version_minor=%d\n", config->version_minor);
    fprintf(file, "version_patch=%d\n", config->version_patch);
    fprintf(file, "last_updated=%s\n", config->last_updated);
    fprintf(file, "workspace=%s\n", config->workspace);
    fclose(file);
    return 1;
}

static void config_defaults(AppConfig *config) {
    memset(config, 0, sizeof(*config));
    snprintf(config->project_name, sizeof(config->project_name), "%s", APP_NAME);
    snprintf(config->project_title, sizeof(config->project_title), "%s", APP_TITLE);
    snprintf(config->description, sizeof(config->description), "%s", APP_DESCRIPTION);
    config->version_major = APP_VERSION_MAJOR;
    config->version_minor = APP_VERSION_MINOR;
    config->version_patch = APP_VERSION_PATCH;
}

int config_load_or_create(AppConfig *config) {
    char path[MAX_PATH_LEN], line[2048];
    FILE *file;
    int first_run = 0;
    config_defaults(config);
    if (!make_config_path(path, sizeof(path))) return 0;
    if (!path_exists(path)) {
        char workspace[MAX_PATH_LEN];
        first_run = 1;
        ui_print_wrapped("First run: choose a workspace directory.");
        ui_print_inlinef("Workspace path: ");
        if (!fgets(workspace, sizeof(workspace), stdin)) return 0;
        workspace[strcspn(workspace, "\r\n")] = '\0';
        trim(workspace);
        if (!workspace[0]) {
            ui_error_wrappedf("Workspace path cannot be empty.");
            return 0;
        }
        path_normalize(workspace);
        if (!ensure_directory(workspace)) {
            ui_error_wrappedf("Cannot create or access the workspace directory.");
            return 0;
        }
        snprintf(config->workspace, sizeof(config->workspace), "%s", workspace);
        {
            char pathbuf[MAX_PATH_LEN];
            const char *names[] = {"Source", "Output", "User"};
            size_t i;
            for (i = 0; i < 3; ++i) {
                if (!path_join(pathbuf, sizeof(pathbuf), workspace, names[i]) ||
                    !ensure_directory(pathbuf)) {
                    ui_error_wrappedf("Unable to create workspace folders.");
                    return 0;
                }
            }
        }
        timestamp_iso(config->last_updated, sizeof(config->last_updated));
        if (!config_save(config)) return 0;
        ui_print_wrapped("Workspace initialized successfully.");
        putchar('\n');
    } else {
        file = fopen(path, "r");
        if (!file) return 0;
        while (fgets(line, sizeof(line), file)) {
            char *eq;
            trim(line);
            if (!line[0] || line[0] == '#') continue;
            eq = strchr(line, '=');
            if (!eq) continue;
            *eq++ = '\0'; trim(line); trim(eq);
            if (!strcmp(line, "project_name")) snprintf(config->project_name, sizeof(config->project_name), "%s", eq);
            else if (!strcmp(line, "project_title")) snprintf(config->project_title, sizeof(config->project_title), "%s", eq);
            else if (!strcmp(line, "description")) snprintf(config->description, sizeof(config->description), "%s", eq);
            else if (!strcmp(line, "run_count")) config->run_count = strtoul(eq, NULL, 10);
            else if (!strcmp(line, "version_major")) config->version_major = atoi(eq);
            else if (!strcmp(line, "version_minor")) config->version_minor = atoi(eq);
            else if (!strcmp(line, "version_patch")) config->version_patch = atoi(eq);
            else if (!strcmp(line, "last_updated")) snprintf(config->last_updated, sizeof(config->last_updated), "%s", eq);
            else if (!strcmp(line, "workspace")) snprintf(config->workspace, sizeof(config->workspace), "%s", eq);
        }
        fclose(file);
        if (!config->workspace[0] || !path_exists(config->workspace)) {
            char workspace[MAX_PATH_LEN];
            ui_print_wrapped("Configured workspace is unavailable.");
            ui_print_inlinef("Workspace path: ");
            if (!fgets(workspace, sizeof(workspace), stdin)) return 0;
            workspace[strcspn(workspace, "\r\n")] = '\0'; trim(workspace); path_normalize(workspace);
            if (!ensure_directory(workspace)) return 0;
            snprintf(config->workspace, sizeof(config->workspace), "%s", workspace);
            {
                char p[MAX_PATH_LEN];
                const char *names[] = {"Source", "Output", "User"};
                size_t i;
                for (i=0; i<3; ++i) {
                    if (!path_join(p, sizeof(p), workspace, names[i]) || !ensure_directory(p)) return 0;
                }
            }
            config_save(config);
        }
    }
    (void)first_run;
    return 1;
}

/* === End of file: config.c === */
