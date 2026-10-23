/*---------------------------------------------------------------*
*  Project Title  : FontToolkit
*  Filename       : main.c
*  Version app    : 1.1.0
*  Version file   : 1.1.1
*  Last Update    : 2026-10-22
*  Description    : Application entry point and primary console menu.
*----------------------------------------------------------------*/
#include "app.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void print_banner(const AppConfig *config) {
    char version[UI_LINE_WIDTH + 1];
    printf("\n");
    ui_print_line('=');
    ui_print_wrappedf("%s", config->project_name);
    ui_print_wrappedf("%s", config->project_title);
    snprintf(version, sizeof(version), "Version %d.%d.%d dc.%lu",
             config->version_major, config->version_minor,
             config->version_patch, config->run_count);
    ui_print_wrapped(version);
    ui_print_line('=');
    ui_print_wrappedf("Workspace: %s", config->workspace[0]
                       ? config->workspace : "UNKWN");
    ui_print_wrappedf("%s%s Status: OK%s", ui.Role.success, ui.Icon.OK, ui.Style.Reset);
    ui_print_wrappedf("Theme: %s", uifx_theme_name());
    putchar('\n');
}

void print_main_menu(void) {
    ui_print_line('=');
    ui_print_wrapped("MAIN MENU");
    ui_print_line('-');
    ui_print_wrapped("1) Scan source folder");
    ui_print_wrapped("2) Rename to original name");
    ui_print_wrapped("3) Group by font family");
    ui_print_wrapped("4) Undo operation output");
    ui_print_wrapped("5) Interface theme settings");
    ui_print_wrapped("X) Exit");
    ui_print_line('-');
    ui_print_inlinef("Choice: ");
}

static void theme_settings(void) {
    char choice[32];
    for (;;) {
        ui_print_line('=');
        ui_print_wrapped("INTERFACE THEME SETTINGS");
        ui_print_line('-');
        ui_print_wrappedf("Current theme: %s", uifx_theme_name());
        ui_print_wrapped("D) Dark theme");
        ui_print_wrapped("L) Light theme");
        ui_print_wrapped("R) Restore built-in defaults");
        ui_print_wrapped("0) Back");
        ui_print_inlinef("Choice: ");
        if (!fgets(choice, sizeof(choice), stdin)) return;
        choice[strcspn(choice, "\r\n")] = '\0';
        if (choice[0]=='0') return;
        if (choice[0]=='d'||choice[0]=='D') { if(uifx_theme_select("dark")) ui_print_wrapped("Dark theme saved."); else ui_error_wrappedf("Unable to save theme settings."); }
        else if (choice[0]=='l'||choice[0]=='L') { if(uifx_theme_select("light")) ui_print_wrapped("Light theme saved."); else ui_error_wrappedf("Unable to save theme settings."); }
        else if (choice[0]=='r'||choice[0]=='R') { if(uifx_theme_reset_defaults()) ui_print_wrapped("Built-in defaults restored."); else ui_error_wrappedf("Unable to save defaults."); }
        else ui_print_wrapped("Invalid choice.");
    }
}

int main(void) {
    AppConfig config;
    char choice[64];

    if (!config_load_or_create(&config)) {
        ui_error_wrappedf("Unable to initialize application configuration.");
        return EXIT_FAILURE;
    }

    (void)uifx_theme_init();
    config.run_count++;
    timestamp_iso(config.last_updated, sizeof(config.last_updated));
    if (!config_save(&config)) {
        ui_error_wrappedf("Warning: unable to update project configuration.");
    }

    for (;;) {
        print_banner(&config);
        print_main_menu();
        if (!fgets(choice, sizeof(choice), stdin)) {
            break;
        }
        choice[strcspn(choice, "\r\n")] = '\0';
        while (choice[0] && isspace((unsigned char)choice[0])) {
            memmove(choice, choice + 1, strlen(choice));
        }

        if (choice[0] == 'x' || choice[0] == 'X') {
            ui_print_wrapped("Goodbye.");
            break;
        } else if (strcmp(choice, "1") == 0) {
            run_scan(&config);
        } else if (strcmp(choice, "2") == 0) {
            run_rename(&config);
        } else if (strcmp(choice, "3") == 0) {
            run_group(&config);
        } else if (strcmp(choice, "4") == 0) {
            run_undo(&config);
        } else if (strcmp(choice, "5") == 0) {
            theme_settings();
        } else {
            ui_print_wrapped("Invalid choice. Select 1-5 or X.");
        }
        ui_print_wrapped("Press Enter to continue...");
        (void)fgets(choice, sizeof(choice), stdin);
    }
    return EXIT_SUCCESS;
}

/* === End of file: main.c === */
