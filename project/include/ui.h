/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : ui.h
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Console UI helpers with a 41-column limit.
*----------------------------------------------------------------*/
#ifndef FONTTOOLKIT_UI_H
#define FONTTOOLKIT_UI_H

#include <stddef.h>
#include "common.h"

#define UI_LINE_WIDTH COMMON_CONSOLE_WIDTH
#define UI_LINE_BUFFER_SIZE (UI_LINE_WIDTH + 1)

char *build_strline(char ch, char out[UI_LINE_BUFFER_SIZE]);
void ui_print_line(char ch);
void ui_print_inlinef(const char *format, ...);
void ui_print_progressf(const char *format, ...);
void ui_print_progress_end(void);
void ui_print_wrapped(const char *text);
void ui_print_wrappedf(const char *format, ...);
void ui_error_wrappedf(const char *format, ...);

#endif /* FONTTOOLKIT_UI_H */
