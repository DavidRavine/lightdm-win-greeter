/* lightdm-mini-greeter - A minimal GTK LightDM Greeter */
#include <sys/mman.h>

#include <gtk/gtk.h>
#include <gtk/gtkx.h>

#include "app.h"
#include "utils.h"


int main(int argc, char **argv)
{
    // This is apparently a bad idea, so we disable it (source: lightdm-gtk-greeter)
    // mlockall(MCL_CURRENT | MCL_FUTURE);  // Keep data out of any swap devices

    App *app = initialize_app(argc, argv);

    if (!connect_to_lightdm_daemon(app->greeter)) {
        return EXIT_FAILURE;
    }

    // Make the greeter behave a bit more like a screensaver if used as un/lock-screen by blanking the screen
    // (source: GTK Greeter)
    if (lightdm_greeter_get_lock_hint(app->greeter)) {
        Display *display = gdk_x11_display_get_xdisplay(gdk_display_get_default());
        XGetScreenSaver(display, &app->timeout, &app->interval, &app->prefer_blanking, &app->allow_exposures);
        XForceScreenSaver(display, ScreenSaverActive);
        XSetScreenSaver(display, 30, 0, ScreenSaverActive, DefaultExposures);
    }

    begin_authentication_as_default_user(app);
    make_session_focus_ring(app);

    for (int m = 0; m < APP_MONITOR_COUNT(app); m++) {
        gtk_widget_show_all(GTK_WIDGET(APP_BACKGROUND_WINDOWS(app)[m]));
    }
    gtk_widget_show_all(GTK_WIDGET(APP_MAIN_WINDOW(app)));
    gtk_window_present(APP_MAIN_WINDOW(app));
    gtk_main();

    destroy_app(app);
}
