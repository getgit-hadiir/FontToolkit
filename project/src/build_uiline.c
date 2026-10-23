/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : build_uiline.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Builds safe fixed-width console separator lines.
*----------------------------------------------------------------*/
#include "ui.h"

#include <stdio.h>
#include <string.h>

char *build_strline(char ch, char out[UI_LINE_BUFFER_SIZE]) {
    if (!out) return NULL;
    memset(out, (unsigned char)ch, UI_LINE_WIDTH);
    out[UI_LINE_WIDTH] = '\0';
    return out;
}

void ui_print_line(char ch) {
    char line[UI_LINE_BUFFER_SIZE];
    if (build_strline(ch, line)) printf("%s\n", line);
}

/* === End of file: build_uiline.c === */
