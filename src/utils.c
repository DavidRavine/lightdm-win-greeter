/* General Utility Functions */
#include <lightdm.h>
#include <glib.h>

#include "app.h"
#include "compat.h"
#include "lightdm/session.h"
#include "utils.h"
#include "focus_ring.h"

static gchar *get_session_key(gconstpointer data);


/* Connect to the LightDM daemon or exit with an error */
void connect_to_lightdm_daemon(LightDMGreeter *greeter)
{
    if (!lightdm_greeter_connect_sync(greeter, NULL)) {
        g_critical("Could not connect to the LightDM daemon");
    }
}


/* Begin authentication as the default user, or exit with an error */
void begin_authentication_as_default_user(App *app)
{
    const gchar *default_user = APP_LOGIN_USER(app);
    if (g_strcmp0(default_user, NULL) == 0) {
        g_critical("A default user has not been not set");
    } else {
        g_message("Beginning authentication as the default user: %s",
                  default_user);
        compat_greeter_authenticate(app->greeter, default_user, NULL);
    }
}


/* Remove every occurence of a character from a string */
void remove_char(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}


/* Get Sessions & Build the Focus Ring */
void make_session_focus_ring(App *app)
{
    const gchar *default_session =
            lightdm_greeter_get_default_session_hint(app->greeter);
    const GList *sessions = lightdm_get_sessions();
    FocusRing *session_ring = initialize_focus_ring(sessions, &get_session_key, "sessions");

    if (default_session != NULL) {
        focus_ring_scroll_to_value(session_ring, default_session);
    }
    g_message("Initial session set to: %s", focus_ring_get_value(session_ring));

    app->session_ring = session_ring;
}
/* Retrieves the `key` field of a session, used to pull current session out of
 * a FocusRing.
 */
static gchar *get_session_key(gconstpointer data)
{
    LightDMSession *session = (LightDMSession *) data;
    return (gchar *) lightdm_session_get_key(session);
}


static void calculate_background_size(GdkPixbufLoader* loader, guint width, guint height, gpointer data)
{
    guint* window_size = (guint*) data;
    // determine optimal image bounds
    int bg_width, bg_height;
    double aspect = (double) width / (double) height;
    double window_aspect = (double) window_size[0] / (double) window_size[1];

    if (aspect < window_aspect) {
        double scale = (double) window_size[0] / (double) width;
        bg_width = (int) (window_size[0]);
        bg_height = (int) (height * scale);
    } else {
        double scale = (double) window_size[1] / (double) height;
        bg_width = (int) (width * scale);
        bg_height = (int) (window_size[1]);
    }

    fprintf(stderr, "[GREETER] setting size %d x %d\n", bg_width, bg_height);
    gdk_pixbuf_loader_set_size(loader, bg_width, bg_height);
}

/* Loads an image to cover the specified size*/
GdkPixbuf* load_image_to_cover(gchar* filename, guint min_width, guint min_height, GError** error)
{
    guint container_size[2] = { min_width, min_height };

    fprintf(stderr, "[GREETER] loading %s\n", filename);
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
    // set the correct size during loading
    g_signal_connect(loader, "size-prepared",
                    G_CALLBACK(calculate_background_size), container_size);
    
    // load image from file
    gchar* file_buffer;
    gsize read_bytes;
    g_file_get_contents(filename, &file_buffer, &read_bytes, error);
    if (error != NULL && *error != NULL)
        return NULL;

    gdk_pixbuf_loader_write(loader, (guchar*)file_buffer, read_bytes, error);
    if (error != NULL && *error != NULL) return NULL;

    g_free(file_buffer);
    gdk_pixbuf_loader_close(loader, NULL);
    if (error != NULL && *error != NULL) return NULL;

    GdkPixbuf* intermediate_result = gdk_pixbuf_loader_get_pixbuf(loader);

    guint width = (guint) gdk_pixbuf_get_width(intermediate_result);
    guint height = (guint) gdk_pixbuf_get_height(intermediate_result);

    if (width < min_width || height < min_height) {
        int bg_width, bg_height;
        double aspect = (double) width / (double) height;
        double window_aspect = (double) min_width / (double) min_height;

        if (aspect < window_aspect) {
            double scale = (double) min_width / (double) width;
            bg_width = (int) (min_width);
            bg_height = (int) (height * scale);
        } else {
            double scale = (double) min_height / (double) height;
            bg_width = (int) (width * scale);
            bg_height = (int) (min_height);
        }

        GdkPixbuf* result = gdk_pixbuf_scale_simple(intermediate_result, bg_width, bg_height, GDK_INTERP_BILINEAR);
        g_object_unref(intermediate_result);
        return result;
    }

    return intermediate_result;
}

