#ifndef LOGIN_UI_H
#define LOGIN_UI_H

#include <gtk/gtk.h>
#include "config.h"

typedef struct LoginUI {
    GtkBox*      login_container;
    GtkWidget*   username_label;

    GtkBox*      password_line;
    GtkWidget*   password_input;
    GtkButton*   login_button;
    GtkWidget*   feedback_label;
} LoginUI;


LoginUI *initialize_login_ui(Config *config);

#endif // LOGIN_UI_H