/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : font_metadata.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : TrueType/OpenType font metadata extraction.
*----------------------------------------------------------------*/
#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned short read_u16be(const unsigned char *p) {
    return (unsigned short)(((unsigned short)p[0] << 8) | p[1]);
}
static unsigned long read_u32be(const unsigned char *p) {
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

static void utf16be_to_utf8(const unsigned char *src, size_t len, char *out, size_t out_size) {
    size_t i = 0, used = 0;
    if (!out_size) return;
    while (i + 1 < len && used + 4 < out_size) {
        unsigned long cp = read_u16be(src + i);
        i += 2;
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < len) {
            unsigned long low = read_u16be(src + i);
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                i += 2;
            }
        }
        if (cp == 0) continue;
        if (cp < 0x80) out[used++] = (char)cp;
        else if (cp < 0x800) {
            out[used++] = (char)(0xC0 | (cp >> 6));
            out[used++] = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out[used++] = (char)(0xE0 | (cp >> 12));
            out[used++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[used++] = (char)(0x80 | (cp & 0x3F));
        } else {
            out[used++] = (char)(0xF0 | (cp >> 18));
            out[used++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[used++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[used++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    out[used] = '\0';
}

static void copy_ascii_or_utf16(const unsigned char *src, size_t len, int platform,
                                char *out, size_t out_size) {
    size_t i, used = 0;
    if (platform == 0 || platform == 3) {
        utf16be_to_utf8(src, len, out, out_size);
        return;
    }
    for (i = 0; i < len && used + 1 < out_size; ++i) {
        unsigned char c = src[i];
        if (c >= 32 && c < 127) out[used++] = (char)c;
        else if (c >= 160) out[used++] = (char)c;
    }
    out[used] = '\0';
}

int read_font_metadata(FontRecord *record) {
    FILE *file;
    unsigned char header[12];
    unsigned long table_count, i, name_offset = 0, name_length = 0;
    unsigned char *table = NULL;
    if (!record) return 0;
    file = fopen(record->path, "rb");
    if (!file) return 0;
    if (fread(header, 1, 12, file) != 12) { fclose(file); return 0; }
    if (memcmp(header, "\x00\x01\x00\x00", 4) != 0 &&
        memcmp(header, "OTTO", 4) != 0 &&
        memcmp(header, "true", 4) != 0 &&
        memcmp(header, "typ1", 4) != 0) {
        fclose(file);
        return 0;
    }
    table_count = read_u16be(header + 4);
    if (table_count == 0 || table_count > 4096) {
        fclose(file);
        return 0;
    }
    for (i = 0; i < table_count; ++i) {
        unsigned char entry[16];
        if (fread(entry, 1, sizeof(entry), file) != sizeof(entry)) break;
        if (!memcmp(entry, "name", 4)) {
            name_offset = read_u32be(entry + 8);
            name_length = read_u32be(entry + 12);
            break;
        }
    }
    if (!name_offset || name_length < 6 || name_length > 16 * 1024 * 1024) {
        fclose(file); return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    {
        long file_size = ftell(file);
        if (file_size < 0 || name_offset > (unsigned long)file_size ||
            name_length > (unsigned long)file_size - name_offset) {
            fclose(file);
            return 0;
        }
    }
    table = (unsigned char *)malloc(name_length);
    if (!table) { fclose(file); return 0; }
    if (fseek(file, (long)name_offset, SEEK_SET) != 0 ||
        fread(table, 1, name_length, file) != name_length) {
        free(table); fclose(file); return 0;
    }
    fclose(file);
    {
        unsigned short count = read_u16be(table + 2);
        unsigned short strings_offset = read_u16be(table + 4);
        unsigned short r;
        int family_best = 0, subfamily_best = 0, full_best = 0, postscript_best = 0;
        size_t records_end = 6u + (size_t)count * 12u;
        if (records_end > name_length || strings_offset > name_length) {
            free(table);
            return 0;
        }
        for (r = 0; r < count; ++r) {
            const unsigned char *rec = table + 6u + (size_t)r * 12u;
            unsigned short platform = read_u16be(rec);
            unsigned short name_id = read_u16be(rec + 6);
            unsigned short length = read_u16be(rec + 8);
            unsigned short offset = read_u16be(rec + 10);
            char decoded[1024];
            int score = 1;
            size_t absolute = (size_t)strings_offset + offset;
            if (absolute > name_length || (size_t)length > name_length - absolute) continue;
            if (platform == 0) score = 4;
            else if (platform == 3) score = 3;
            else if (platform == 1) score = 1;
            else continue;
            copy_ascii_or_utf16(table + absolute, length, platform, decoded, sizeof(decoded));
            if (!decoded[0]) continue;
            /* Typographic family/subfamily (IDs 16/17) take precedence over legacy IDs 1/2. */
            if (name_id == 1 || name_id == 16) {
                int adjusted = score + (name_id == 16 ? 10 : 0);
                if (adjusted > family_best) {
                    snprintf(record->family, sizeof(record->family), "%.*s", (int)sizeof(record->family) - 1, decoded);
                    family_best = adjusted;
                }
            } else if (name_id == 2 || name_id == 17) {
                int adjusted = score + (name_id == 17 ? 10 : 0);
                if (adjusted > subfamily_best) {
                    snprintf(record->subfamily, sizeof(record->subfamily), "%.*s", (int)sizeof(record->subfamily) - 1, decoded);
                    subfamily_best = adjusted;
                }
            } else if (name_id == 4 && score > full_best) {
                snprintf(record->full_name, sizeof(record->full_name), "%.*s", (int)sizeof(record->full_name) - 1, decoded);
                full_best = score;
            } else if (name_id == 6 && score > postscript_best) {
                snprintf(record->postscript_name, sizeof(record->postscript_name), "%.*s", (int)sizeof(record->postscript_name) - 1, decoded);
                postscript_best = score;
            }
        }
        if (family_best == 0) snprintf(record->family, sizeof(record->family), "Unknown");
        if (full_best == 0 && family_best > 0) {
            if (record->subfamily[0] && strcmp(record->subfamily, "Regular") != 0)
                snprintf(record->full_name, sizeof(record->full_name), "%.*s %.*s", 250, record->family, 250, record->subfamily);
            else snprintf(record->full_name, sizeof(record->full_name), "%s", record->family);
        }
    }
    free(table);
    return record->family[0] != '\0' && strcmp(record->family, "Unknown") != 0;
}

/* === End of file: font_metadata.c === */
