#ifndef UTILS_H
#define UTILS_H

#include <lightdm.h>
#include <gtk/gtk.h>

#include "app.h"


gboolean connect_to_lightdm_daemon(LightDMGreeter *greeter);
void make_session_focus_ring(App *app);
void begin_authentication_as_default_user(App *app);
void remove_char(char *str, char garbage);
GdkPixbuf* load_image_to_cover(gchar* filename, guint min_width, guint min_height, GError** error);

#endif
