/* Callback Functions for LightDM & GTK */
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <lightdm.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "utils.h"
#include "focus_ring.h"
#include "callbacks.h"
#include "compat.h"
#include "ui.h"

static void set_ui_feedback_label(App *app, gchar *feedback_text);


/* LightDM Callbacks */

/* Start the Selected Session Once Fully Authenticated.
 *
 * The callback will clear & re-enable the input widget, and re-add the
 * `handle_password` callback so the user can try again if authentication
 * fails.
 */
void authentication_complete_cb(LightDMGreeter *greeter, App *app)
{
    if (lightdm_greeter_get_is_authenticated(greeter)) {
        const gchar *session = focus_ring_get_value(app->session_ring);

        g_message("Attempting to start session: %s", session);

        gboolean session_started_successfully =
            !lightdm_greeter_start_session_sync(greeter, session, NULL);

        if (!session_started_successfully) {
            g_message("Unable to start session");
        }
    } else {
        g_message("Authentication failed");
        if (strlen(app->config->invalid_password_text) > 0) {
            set_ui_feedback_label(app, app->config->invalid_password_text);
        }
        begin_authentication_as_default_user(app);
    }
    gtk_entry_set_text(GTK_ENTRY(APP_PASSWORD_INPUT(app)), "");
    gtk_editable_set_editable(GTK_EDITABLE(APP_PASSWORD_INPUT(app)), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(APP_LOGIN_BUTTON(app)), TRUE);
    app->password_callback_id =
        g_signal_connect(GTK_ENTRY(APP_PASSWORD_INPUT(app)), "activate",
                         G_CALLBACK(handle_password), app);
    app->button_password_callback_id =
        g_signal_connect(GTK_BUTTON(APP_LOGIN_BUTTON(app)), "clicked",
                         G_CALLBACK(handle_password), app);
}



/* GUI Callbacks */

/* Attempt to Authenticate When a Password is Entered.
 *
 * The callback disables itself & the input widget to prevent two
 * authentication attempts from running at the same time - which would cause
 * LightDM to throw a critical error.
 */
void handle_password(GtkWidget *password_input, App *app)
{
    // Reset to default screensaver values (source GTK Greeter)
    if (lightdm_greeter_get_lock_hint(app->greeter)) {
        XSetScreenSaver(gdk_x11_display_get_xdisplay(gdk_display_get_default()), app->timeout, app->interval, app->prefer_blanking, app->allow_exposures);
    }

    if (app->password_callback_id != 0) {
        g_signal_handler_disconnect(GTK_ENTRY(APP_PASSWORD_INPUT(app)),
                                    app->password_callback_id);
        g_signal_handler_disconnect(GTK_BUTTON(APP_LOGIN_BUTTON(app)),
                                    app->button_password_callback_id);
        app->password_callback_id = 0;
        app->button_password_callback_id = 0;
    }

    if (!lightdm_greeter_get_is_authenticated(app->greeter)) {
        gtk_editable_set_editable(GTK_EDITABLE(APP_PASSWORD_INPUT(app)), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(APP_LOGIN_BUTTON(app)), FALSE);

        if (!lightdm_greeter_get_in_authentication(app->greeter)) {
            begin_authentication_as_default_user(app);
        }
        g_message("Using entered password to authenticate");
        const gchar *password_text =
            gtk_entry_get_text(GTK_ENTRY(APP_PASSWORD_INPUT(app)));
        compat_greeter_respond(app->greeter, password_text, NULL);
    } else {
        g_message("Password entered while already authenticated");
    }
}


/* Select the Password input if the Tab Key is Pressed */
gboolean handle_tab_key(GtkWidget *widget, GdkEvent *event, App *app)
{
    (void) widget;  // Window accessible through app.

    GdkEventKey *key_event = (GdkEventKey *) event;
    if (event->type == GDK_KEY_PRESS && key_event->keyval == GDK_KEY_Tab) {
        g_message("Handling Tab Key Press");
        gtk_window_present(GTK_WINDOW(APP_MAIN_WINDOW(app)));
        gtk_widget_grab_focus(GTK_WIDGET(APP_PASSWORD_INPUT(app)));
        return FALSE;
    }
    return TRUE;
}

/* Shutdown, Restart, Hibernate, Suspend, or Switch Sessions if the correct
 * keys are pressed.
 */
gboolean handle_hotkeys(GtkWidget *widget, GdkEventKey *event, App *app)
{
    (void) widget;
    Config *config = app->config;
    FocusRing *sessions = app->session_ring;

    if (event->state & config->mod_bit) {
        if (event->keyval == config->suspend_key && lightdm_get_can_suspend()) {
            lightdm_suspend(NULL);
        } else if (event->keyval == config->hibernate_key &&
                   lightdm_get_can_hibernate()) {
            lightdm_hibernate(NULL);
        } else if (event->keyval == config->restart_key &&
                   lightdm_get_can_restart()) {
            lightdm_restart(NULL);
        } else if (event->keyval == config->shutdown_key &&
                   lightdm_get_can_shutdown()) {
            lightdm_shutdown(NULL);
        } else if (event->keyval == config->session_key && sessions != NULL) {
            gchar *new_session = focus_ring_next(sessions);
            set_ui_feedback_label(app, new_session);
        } else {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}

gboolean handle_cover_uncover(GtkWidget *widget, GdkEventKey *event, App *app)
{
    if (app->state == APP_COVERED) {
        app->state = APP_LOGIN;
        ui_uncover(app->ui);
        return TRUE;
        
    } else if (event->keyval == GDK_KEY_Escape) {
        app->state = APP_COVERED;
        ui_cover(app->ui);
    }

    return FALSE;
}

/** Determine the current time & update the time GtkLabel.
 */
gboolean handle_time_update(App *app)
{
    time_t now = time(NULL);
    struct tm *local_now = localtime(&now);
    gchar time_string[30];
    gchar date_string[50];
    strftime(time_string, 29, "%H:%M", local_now);
    strftime(date_string, 29, "%A, %d.%m.%Y", local_now);
    gtk_label_set_text(GTK_LABEL(APP_TIME_LABEL(app)), time_string);
    gtk_label_set_text(GTK_LABEL(APP_DATE_LABEL(app)), date_string);

    return TRUE;
}

/* Set the Feedback Label's text & ensure it is visible. */
static void set_ui_feedback_label(App *app, gchar *feedback_text)
{
    if (!gtk_widget_get_visible(APP_FEEDBACK_LABEL(app))) {
        gtk_widget_show(APP_FEEDBACK_LABEL(app));
    }
    gtk_label_set_text(GTK_LABEL(APP_FEEDBACK_LABEL(app)), feedback_text);
}

void power_shutdown(GtkWidget* item)
{
    lightdm_shutdown(NULL);
}
void power_restart(GtkWidget* item)
{
    lightdm_restart(NULL);
}
void power_suspend(GtkWidget* item)
{
    lightdm_suspend(NULL);
}
