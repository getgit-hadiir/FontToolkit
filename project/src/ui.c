/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : ui.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Console UI, fixed-width lines, wrapping, progress.
*----------------------------------------------------------------*/
#include "ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static size_t ui_utf8_bytes(const unsigned char *p) {
    if ((p[0] & 0x80u) == 0) return 1;
    if ((p[0] & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u) return 2;
    if ((p[0] & 0xF0u) == 0xE0u &&
        (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u) return 3;
    if ((p[0] & 0xF8u) == 0xF0u &&
        (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u &&
        (p[3] & 0xC0u) == 0x80u) return 4;
    return 1;
}

static unsigned long ui_utf8_codepoint(const unsigned char *p, size_t bytes) {
    if (bytes == 1) return p[0];
    if (bytes == 2) return ((unsigned long)(p[0] & 0x1Fu) << 6) |
                           (unsigned long)(p[1] & 0x3Fu);
    if (bytes == 3) return ((unsigned long)(p[0] & 0x0Fu) << 12) |
                           ((unsigned long)(p[1] & 0x3Fu) << 6) |
                           (unsigned long)(p[2] & 0x3Fu);
    return ((unsigned long)(p[0] & 0x07u) << 18) |
           ((unsigned long)(p[1] & 0x3Fu) << 12) |
           ((unsigned long)(p[2] & 0x3Fu) << 6) |
           (unsigned long)(p[3] & 0x3Fu);
}

static size_t ui_utf8_cell_width(const unsigned char *p, size_t bytes) {
    unsigned long cp = ui_utf8_codepoint(p, bytes);
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F) ||
        (cp >= 0xFE20 && cp <= 0xFE2F)) return 0;
    if (cp >= 0x1100 &&
        (cp <= 0x115F || cp == 0x2329 || cp == 0x232A ||
         (cp >= 0x2E80 && cp <= 0xA4CF) ||
         (cp >= 0xAC00 && cp <= 0xD7A3) ||
         (cp >= 0xF900 && cp <= 0xFAFF) ||
         (cp >= 0xFE10 && cp <= 0xFE19) ||
         (cp >= 0xFE30 && cp <= 0xFE6F) ||
         (cp >= 0xFF00 && cp <= 0xFF60) ||
         (cp >= 0xFFE0 && cp <= 0xFFE6) ||
         (cp >= 0x1F300 && cp <= 0x1FAFF) ||
         (cp >= 0x20000 && cp <= 0x3FFFD))) return 2;
    return 1;
}

static size_t ui_text_cell_width(const unsigned char *begin, const unsigned char *end) {
    size_t width = 0;
    while (begin < end) {
        size_t bytes = ui_utf8_bytes(begin);
        if (begin + bytes > end) bytes = 1;
        width += ui_utf8_cell_width(begin, bytes);
        begin += bytes;
    }
    return width;
}

static void ui_vprint_wrapped(FILE *stream, const char *text) {
    size_t column = 0;
    const unsigned char *p = (const unsigned char *)text;
    if (!text) return;

    while (*p) {
        const unsigned char *word_end;
        size_t word_width;

        if (*p == '\n') {
            fputc('\n', stream);
            column = 0;
            ++p;
            continue;
        }
        if (*p == ' ' || *p == '\t' || *p == '\r') {
            while (*p == ' ' || *p == '\t' || *p == '\r') ++p;
            if (!*p || *p == '\n') continue;
            if (column) {
                if (column >= UI_LINE_WIDTH) {
                    fputc('\n', stream);
                    column = 0;
                } else {
                    fputc(' ', stream);
                    ++column;
                }
            }
            continue;
        }

        word_end = p;
        while (*word_end && *word_end != ' ' && *word_end != '\t' &&
               *word_end != '\r' && *word_end != '\n') ++word_end;
        word_width = ui_text_cell_width(p, word_end);
        if (column && column + word_width > UI_LINE_WIDTH) {
            fputc('\n', stream);
            column = 0;
        }

        while (p < word_end) {
            size_t bytes = ui_utf8_bytes(p);
            size_t cells;
            if (p + bytes > word_end) bytes = 1;
            cells = ui_utf8_cell_width(p, bytes);
            if (cells && column + cells > UI_LINE_WIDTH) {
                fputc('\n', stream);
                column = 0;
            }
            fwrite(p, 1, bytes, stream);
            column += cells;
            p += bytes;
        }
    }
    if (column || text[0] == '\0') fputc('\n', stream);
}

static int ui_format(char *buffer, size_t size, const char *format, va_list args) {
    int written;
    if (!buffer || size == 0 || !format) return 0;
    written = vsnprintf(buffer, size, format, args);
    buffer[size - 1] = '\0';
    return written >= 0;
}

void ui_print_wrapped(const char *text) {
    ui_vprint_wrapped(stdout, text);
}

void ui_print_wrappedf(const char *format, ...) {
    char buffer[8192];
    va_list args;
    va_start(args, format);
    if (ui_format(buffer, sizeof(buffer), format, args)) ui_vprint_wrapped(stdout, buffer);
    va_end(args);
}

void ui_error_wrappedf(const char *format, ...) {
    char buffer[8192];
    va_list args;
    va_start(args, format);
    if (ui_format(buffer, sizeof(buffer), format, args)) ui_vprint_wrapped(stderr, buffer);
    va_end(args);
}

void ui_print_inlinef(const char *format, ...) {
    char buffer[8192];
    va_list args;
    size_t length;
    va_start(args, format);
    if (!ui_format(buffer, sizeof(buffer), format, args)) {
        va_end(args);
        return;
    }
    va_end(args);
    length = strlen(buffer);
    if (length > UI_LINE_WIDTH) length = UI_LINE_WIDTH;
    fwrite(buffer, 1, length, stdout);
    fflush(stdout);
}

void ui_print_progressf(const char *format, ...) {
    char buffer[8192];
    char line[UI_LINE_BUFFER_SIZE];
    va_list args;
    size_t length;
    va_start(args, format);
    if (!ui_format(buffer, sizeof(buffer), format, args)) {
        va_end(args);
        return;
    }
    va_end(args);
    length = strlen(buffer);
    if (length > UI_LINE_WIDTH) length = UI_LINE_WIDTH;
    memset(line, ' ', UI_LINE_WIDTH);
    memcpy(line, buffer, length);
    line[UI_LINE_WIDTH] = '\0';
    printf("\r%s", line);
    fflush(stdout);
}

void ui_print_progress_end(void) {
    putchar('\n');
    fflush(stdout);
}

/* === End of file: ui.c === */
