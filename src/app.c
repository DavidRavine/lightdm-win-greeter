/* Application Initialization Things */
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <lightdm.h>

#include "app.h"
#include "callbacks.h"
#include "config.h"

/* Initialize the Greeter & UI */
App *initialize_app(int argc, char **argv)
{
    
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
    gtk_init(&argc, &argv);

    // Allocate & Initialize
    App *app = malloc(sizeof(App));
    if (app == NULL) {
        g_error("Could not allocate memory for App");
    }

    app->config = initialize_config();
    app->current_user = app->config->login_user;
    app->greeter = lightdm_greeter_new();
    app->ui = initialize_ui(app->config);
    app->state = APP_COVERED;

    // Connect Greeter & UI Signals
    g_signal_connect(app->greeter, "authentication-complete",
                     G_CALLBACK(authentication_complete_cb), app);
    
    // Make the greeter behave a bit more like a screensaver if used as un/lock-screen by blanking the screen
    // (source: GTK Greeter)
    if (lightdm_greeter_get_lock_hint (app->greeter))
    {
        Display *display = gdk_x11_display_get_xdisplay (gdk_display_get_default ());
        XGetScreenSaver (display, &app->timeout, &app->interval, &app->prefer_blanking, &app->allow_exposures);
        XForceScreenSaver (display, ScreenSaverActive);
        XSetScreenSaver (display, 30, 0, ScreenSaverActive, DefaultExposures);
    }

    app->password_callback_id =
        g_signal_connect(GTK_ENTRY(APP_PASSWORD_INPUT(app)), "activate",
                         G_CALLBACK(handle_password), app);
    app->button_password_callback_id =
        g_signal_connect(GTK_BUTTON(APP_LOGIN_BUTTON(app)), "clicked",
                         G_CALLBACK(handle_password), app);
    // This was added to fix a bug where the background window would be focused
    // instead of the main window, preventing users from entering their password.
    // It's undocument & probably not necessary any more. Investigate & remove.
    for (int m = 0; m < APP_MONITOR_COUNT(app); m++) {
        g_signal_connect(GTK_WIDGET(APP_BACKGROUND_WINDOWS(app)[m]),
                         "key-press-event",
                         G_CALLBACK(handle_tab_key), app);
    }
    g_signal_connect(GTK_WIDGET(APP_MAIN_WINDOW(app)), "key-press-event",
                     G_CALLBACK(handle_hotkeys), app);
                     
    g_signal_connect(GTK_WIDGET(APP_MAIN_WINDOW(app)), "key-press-event",
                     G_CALLBACK(handle_cover_uncover), app);
    g_signal_connect(GTK_WIDGET(APP_MAIN_WINDOW(app)), "button-press-event",
                     G_CALLBACK(handle_cover_uncover), app);
                     
    // Update the current time every 15 seconds
    handle_time_update(app);
    g_timeout_add_seconds(15, G_SOURCE_FUNC(handle_time_update), app);

    return app;
}


/* Free any dynamically allocated memory */
void destroy_app(App *app)
{
    destroy_config(app->config);
    free(app->ui);
    free(app);
}
