/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : uifx.h
*  Version app    : 1.1.1
*  Version file   : 1.2.0
*  Last Update    : 2026-10-22
*  Description    : Central terminal styles, palettes, and theme API.
*----------------------------------------------------------------*/
#ifndef FONTTOOLKIT_UIFX_H
#define FONTTOOLKIT_UIFX_H

typedef struct { const char *Reset, *Bold, *Dim, *Underline; } UiFxStyle;
typedef struct {
    const char *Black, *Red, *Green, *Yellow, *Blue, *Magenta, *Cyan, *White;
    const char *BrightBlack, *BrightRed, *BrightGreen, *BrightYellow;
    const char *BrightBlue, *BrightMagenta, *BrightCyan, *BrightWhite;
} UiFxColor;
typedef struct { const char *OK, *Error, *Warning, *Info, *Arrow, *Bullet, *Check, *Cross; } UiFxIcon;
typedef struct {
    const char *background, *text, *muted_text, *accent, *success;
    const char *warning, *error, *border, *input, *selection;
} UiFxRole;
typedef struct { UiFxStyle Style; UiFxColor Fg, Bg; UiFxIcon Icon; UiFxRole Role; } UiFxNamespace;

extern UiFxNamespace ui;
void uifx_set_ansi_enabled(int enabled);
int uifx_theme_init(void);
int uifx_theme_select(const char *theme_name);
const char *uifx_theme_name(void);
const char *uifx_theme_config_path(void);
int uifx_theme_reset_defaults(void);

#endif /* FONTTOOLKIT_UIFX_H */
