#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "ui_login.h"
#include "config.h"

#define OVERLAY_DEBUG 0

struct BackgroundPixbuf {
    GdkPixbuf* buf;
    gdouble x;
    gdouble y;
};


typedef struct UI_ {
    GtkWindow   **background_windows;
    int         monitor_count;
    GtkWindow   *main_window;
    GtkStack    *layout_stack;

    GtkBox      *layout;
    GtkBox      *layout_vertical;

    GtkGrid     *overlay_container;
    GtkWidget   *time_label;
    GtkWidget   *date_label;
    GtkWidget   *battery_display;
    GtkWidget   *network_display;

    LoginUI     *login_ui;

    struct BackgroundPixbuf* overlay_bg;
    struct BackgroundPixbuf* login_bg;
} UI;


UI *initialize_ui(Config *config);

#endif
