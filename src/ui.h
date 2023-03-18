#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "config.h"


typedef struct UI_ {
    GtkWindow   **background_windows;
    int         monitor_count;
    GtkWindow   *main_window;
    GtkBox      *layout_parent;
    GtkStack    *layout_stack;

    GtkLayout   *layout_container;
    GtkGrid     *login_container;
    GtkImage    *login_background;
    GtkGrid     *overlay_container;
    GtkGrid     *info_container;
    GtkWidget   *sys_info_label;
    GtkWidget   *time_label;
    GtkWidget   *password_label;
    GtkWidget   *password_input;
    GtkWidget   *feedback_label;
} UI;


UI *initialize_ui(Config *config);

#endif
