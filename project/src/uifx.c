/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : uifx.c
*  Version app    : 1.1.1
*  Version file   : 1.2.0
*  Last Update    : 2026-10-22
*  Description    : ANSI palette, editable Dark/Light themes, fallback.
*----------------------------------------------------------------*/
#include "uifx.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESC(code) "\033[" code "m"
#define FG(code) ESC(code)
#define BG(code) ESC(code)
#define ROLE_COUNT 10

typedef struct { const char *name; const char *fg; const char *bg; } ColorDef;
static const ColorDef colors[] = {
 {"BLACK","30","40"},{"RED","31","41"},{"GREEN","32","42"},{"YELLOW","33","43"},
 {"BLUE","34","44"},{"MAGENTA","35","45"},{"CYAN","36","46"},{"WHITE","37","47"},
 {"GRAY","90","100"},{"BRIGHT_BLACK","90","100"},{"BRIGHT_RED","91","101"},
 {"BRIGHT_GREEN","92","102"},{"BRIGHT_YELLOW","93","103"},{"BRIGHT_BLUE","94","104"},
 {"BRIGHT_MAGENTA","95","105"},{"BRIGHT_CYAN","96","106"},{"BRIGHT_WHITE","97","107"},
 {"DARK_GREEN","32","42"},{"DARK_YELLOW","33","43"},{"DARK_RED","31","41"}
};
static const char *role_names[ROLE_COUNT] = {"background","text","muted_text","accent","success","warning","error","border","input","selection"};
static const char *dark_defaults[ROLE_COUNT] = {"BLACK","WHITE","GRAY","CYAN","GREEN","YELLOW","RED","CYAN","WHITE","CYAN"};
static const char *light_defaults[ROLE_COUNT] = {"WHITE","BLACK","GRAY","BLUE","DARK_GREEN","DARK_YELLOW","DARK_RED","GRAY","BLACK","BLUE"};
static char palette[2][ROLE_COUNT][24];
static char active_theme[8] = "dark";
static int ansi_enabled = 1;
static char config_path[4096] = "user/ui_theme.conf";
static const UiFxIcon icons = {"[OK]","[ERROR]","[WARN]","[INFO]","->","*","[v]","[x]"};
UiFxNamespace ui;

static void copy_text(char *dst, size_t n, const char *src) { snprintf(dst,n,"%s",src ? src : ""); }
static void defaults(void) {
    int t, r;
    for (t=0;t<2;t++) for (r=0;r<ROLE_COUNT;r++) copy_text(palette[t][r],sizeof(palette[t][r]), t ? light_defaults[r] : dark_defaults[r]);
}
static void trim(char *s) {
    char *p=s; size_t n;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p!=s) memmove(s,p,strlen(p)+1);
    n=strlen(s); while(n && isspace((unsigned char)s[n-1])) s[--n]='\0';
}
static void upper(char *s) { for (;*s;s++) *s=(char)toupper((unsigned char)*s); }
static int color_valid(const char *name) { size_t i; for(i=0;i<sizeof(colors)/sizeof(colors[0]);i++) if(!strcmp(name,colors[i].name)) return 1; return 0; }
static const char *color_code(const char *name, int background) {
    size_t i; for(i=0;i<sizeof(colors)/sizeof(colors[0]);i++) if(!strcmp(name,colors[i].name)) return background?colors[i].bg:colors[i].fg;
    return background?"40":"30";
}
static const char *ansi(const char *code) {
    if (!ansi_enabled) return "";
    if (!strcmp(code,"30")) return "\033[30m";
    if (!strcmp(code,"31")) return "\033[31m";
    if (!strcmp(code,"32")) return "\033[32m";
    if (!strcmp(code,"33")) return "\033[33m";
    if (!strcmp(code,"34")) return "\033[34m";
    if (!strcmp(code,"35")) return "\033[35m";
    if (!strcmp(code,"36")) return "\033[36m";
    if (!strcmp(code,"37")) return "\033[37m";
    if (!strcmp(code,"40")) return "\033[40m";
    if (!strcmp(code,"41")) return "\033[41m";
    if (!strcmp(code,"42")) return "\033[42m";
    if (!strcmp(code,"43")) return "\033[43m";
    if (!strcmp(code,"44")) return "\033[44m";
    if (!strcmp(code,"45")) return "\033[45m";
    if (!strcmp(code,"46")) return "\033[46m";
    if (!strcmp(code,"47")) return "\033[47m";
    if (!strcmp(code,"90")) return "\033[90m";
    if (!strcmp(code,"91")) return "\033[91m";
    if (!strcmp(code,"92")) return "\033[92m";
    if (!strcmp(code,"93")) return "\033[93m";
    if (!strcmp(code,"94")) return "\033[94m";
    if (!strcmp(code,"95")) return "\033[95m";
    if (!strcmp(code,"96")) return "\033[96m";
    if (!strcmp(code,"97")) return "\033[97m";
    if (!strcmp(code,"100")) return "\033[100m";
    if (!strcmp(code,"101")) return "\033[101m";
    if (!strcmp(code,"102")) return "\033[102m";
    if (!strcmp(code,"103")) return "\033[103m";
    if (!strcmp(code,"104")) return "\033[104m";
    if (!strcmp(code,"105")) return "\033[105m";
    if (!strcmp(code,"106")) return "\033[106m";
    if (!strcmp(code,"107")) return "\033[107m";
    return "";
}
static void apply(void) {
    int i, t=!strcmp(active_theme,"light");
    static char role_esc[ROLE_COUNT][8];
    ui.Style=(UiFxStyle){ansi_enabled?"\033[0m":"",ansi_enabled?"\033[1m":"",ansi_enabled?"\033[2m":"",ansi_enabled?"\033[4m":""};
    ui.Fg=(UiFxColor){ansi(color_code("BLACK",0)),ansi(color_code("RED",0)),ansi(color_code("GREEN",0)),ansi(color_code("YELLOW",0)),ansi(color_code("BLUE",0)),ansi(color_code("MAGENTA",0)),ansi(color_code("CYAN",0)),ansi(color_code("WHITE",0)),ansi(color_code("GRAY",0)),ansi(color_code("BRIGHT_RED",0)),ansi(color_code("BRIGHT_GREEN",0)),ansi(color_code("BRIGHT_YELLOW",0)),ansi(color_code("BRIGHT_BLUE",0)),ansi(color_code("BRIGHT_MAGENTA",0)),ansi(color_code("BRIGHT_CYAN",0)),ansi(color_code("BRIGHT_WHITE",0))};
    ui.Bg=(UiFxColor){ansi(color_code("BLACK",1)),ansi(color_code("RED",1)),ansi(color_code("GREEN",1)),ansi(color_code("YELLOW",1)),ansi(color_code("BLUE",1)),ansi(color_code("MAGENTA",1)),ansi(color_code("CYAN",1)),ansi(color_code("WHITE",1)),ansi(color_code("GRAY",1)),ansi(color_code("BRIGHT_RED",1)),ansi(color_code("BRIGHT_GREEN",1)),ansi(color_code("BRIGHT_YELLOW",1)),ansi(color_code("BRIGHT_BLUE",1)),ansi(color_code("BRIGHT_MAGENTA",1)),ansi(color_code("BRIGHT_CYAN",1)),ansi(color_code("BRIGHT_WHITE",1))};
    for(i=0;i<ROLE_COUNT;i++) { copy_text(role_esc[i],sizeof(role_esc[i]),ansi(color_code(palette[t][i],!strcmp(role_names[i],"background")))); }
    ui.Role=(UiFxRole){role_esc[0],role_esc[1],role_esc[2],role_esc[3],role_esc[4],role_esc[5],role_esc[6],role_esc[7],role_esc[8],role_esc[9]};
    ui.Icon=icons;
}
static int save_config(void) {
    FILE *f; int t,r;
    f=fopen(config_path,"w"); if(!f) return 0;
    fputs("# FontToolkit terminal theme settings\n[General]\n",f); fprintf(f,"theme=%s\n\n",active_theme);
    for(t=0;t<2;t++){fprintf(f,"[%s]\n",t?"Light":"Dark");for(r=0;r<ROLE_COUNT;r++)fprintf(f,"%s=%s\n",role_names[r],palette[t][r]);fputc('\n',f);} fclose(f); return 1;
}
int uifx_theme_init(void) {
    FILE *f; char line[256], section[16]=""; int t=-1, r; defaults();
    f=fopen(config_path,"r");
    if(f){ while(fgets(line,sizeof(line),f)){char *eq; trim(line); if(!line[0]||line[0]=='#'||line[0]==';')continue;
      if(line[0]=='['){char *end=strchr(line,']'); if(!end)continue; *end='\0'; copy_text(section,sizeof(section),line+1); if(!strcmp(section,"Dark"))t=0;else if(!strcmp(section,"Light"))t=1;else t=-1; continue;}
      eq=strchr(line,'=');if(!eq)continue;*eq++='\0';trim(line);trim(eq); upper(eq);
      if(!strcmp(section,"General")&&!strcmp(line,"theme")){if(!strcmp(eq,"DARK")||!strcmp(eq,"LIGHT"))copy_text(active_theme,sizeof(active_theme),!strcmp(eq,"LIGHT")?"light":"dark");continue;}
      if(t>=0&&color_valid(eq))for(r=0;r<ROLE_COUNT;r++)if(!strcmp(line,role_names[r]))copy_text(palette[t][r],sizeof(palette[t][r]),eq);
    } fclose(f); } else { (void)save_config(); }
    apply(); return 1;
}
int uifx_theme_select(const char *name) { if(!name)return 0; if(!strcmp(name,"dark")||!strcmp(name,"DARK"))copy_text(active_theme,sizeof(active_theme),"dark");else if(!strcmp(name,"light")||!strcmp(name,"LIGHT"))copy_text(active_theme,sizeof(active_theme),"light");else return 0; apply(); return save_config(); }
const char *uifx_theme_name(void){return !strcmp(active_theme,"light")?"Light":"Dark";}
const char *uifx_theme_config_path(void){return config_path;}
int uifx_theme_reset_defaults(void){defaults();copy_text(active_theme,sizeof(active_theme),"dark");apply();return save_config();}
void uifx_set_ansi_enabled(int enabled){ansi_enabled=!!enabled;apply();}
/* === End of file: uifx.c === */
