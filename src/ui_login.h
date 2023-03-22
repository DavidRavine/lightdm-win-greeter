#ifndef LOGIN_UI_H
#define LOGIN_UI_H

#include <gtk/gtk.h>
#include "config.h"

typedef struct LoginUI {
    GtkGrid     *login_container;
    GtkWidget   *password_label;
    GtkWidget   *password_input;
    GtkWidget   *feedback_label;

    GtkGrid     *info_container;
    GtkWidget   *sys_info_label;
} LoginUI;


LoginUI *initialize_login_ui(Config *config);

#endif // LOGIN_UI_H