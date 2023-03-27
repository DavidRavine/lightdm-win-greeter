#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <lightdm.h>

#include "app.h"


void authentication_complete_cb(LightDMGreeter* greeter, App* app);
void handle_password(GtkWidget* password_input, App* app);
gboolean handle_tab_key(GtkWidget* widget, GdkEvent* event, App* app);
gboolean handle_hotkeys(GtkWidget* widget, GdkEventKey* event, App* app);
gboolean handle_time_update(App* app);
gboolean handle_cover_uncover(GtkWidget* widget, GdkEventKey* event, App* app);

void power_shutdown(GtkWidget* item);
void power_restart(GtkWidget* item);
void power_suspend(GtkWidget* item);

#endif
