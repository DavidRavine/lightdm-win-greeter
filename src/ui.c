/* Functions related to the GUI. */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <glib.h>
#include <lightdm.h>

#include "callbacks.h"
#include "ui.h"
#include "utils.h"

#define UI_STACK_OVERLAY "overlay"
#define UI_STACK_LOGIN "login"


static UI *new_ui(void);
static void setup_background_windows(Config *config, UI *ui);
static GtkWindow *new_background_window(GdkMonitor *monitor);
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window);
static void hide_mouse_cursor(GtkWidget *window, gpointer user_data);
static void move_mouse_to_background_window(void);
static void setup_main_window(Config *config, UI *ui);
static void place_main_window(GtkWidget *main_window, gpointer user_data);
static void create_and_attach_layout_stack(UI *ui);
static void create_and_attach_overlay_container(UI *ui);
static void create_and_attach_layout_container(UI *ui);
static void create_and_attach_sys_info_label(Config *config, UI *ui);
static void create_and_attach_password_field(Config *config, UI *ui);
static void create_and_attach_feedback_label(UI *ui);
static void attach_config_colors_to_screen(Config *config);


/* Initialize the Main Window & it's Children */
UI *initialize_ui(Config *config)
{
    UI *ui = new_ui();

    setup_background_windows(config, ui);
    move_mouse_to_background_window();
    setup_main_window(config, ui);
    create_and_attach_layout_stack(ui);

    create_and_attach_overlay_container(ui);
    create_and_attach_layout_container(ui);

    gtk_stack_set_visible_child_full(ui->layout_stack, UI_STACK_OVERLAY, GTK_STACK_TRANSITION_TYPE_OVER_DOWN);

    create_and_attach_sys_info_label(config, ui);
    create_and_attach_password_field(config, ui);
    create_and_attach_feedback_label(ui);
    attach_config_colors_to_screen(config);

    return ui;
}


/* Create a new UI with all values initialized to NULL */
static UI *new_ui(void)
{
    UI *ui = malloc(sizeof(UI));
    if (ui == NULL) {
        g_error("Could not allocate memory for UI");
    }
    ui->background_windows = NULL;
    ui->monitor_count = 0;
    ui->main_window = NULL;
    ui->layout_container = NULL;
    ui->password_label = NULL;
    ui->password_input = NULL;
    ui->feedback_label = NULL;

    return ui;
}


/* Create a Background Window for Every Monitor */
static void setup_background_windows(Config *config, UI *ui)
{
    GdkDisplay *display = gdk_display_get_default();
    ui->monitor_count = gdk_display_get_n_monitors(display);
    ui->background_windows = malloc((uint) ui->monitor_count * sizeof (GtkWindow *));
    for (int m = 0; m < ui->monitor_count; m++) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, m);
        if (monitor == NULL) {
            break;
        }

        GtkWindow *background_window = new_background_window(monitor);
        ui->background_windows[m] = background_window;

        gboolean show_background_image =
            (gdk_monitor_is_primary(monitor) || config->show_image_on_all_monitors) &&
            (strcmp(config->background_image, "\"\"") != 0);
        if (show_background_image) {
            GtkStyleContext *style_context =
                gtk_widget_get_style_context(GTK_WIDGET(background_window));
            gtk_style_context_add_class(style_context, "with-image");
        }
    }
}


/* Create & Configure a Background Window for a Monitor */
static GtkWindow *new_background_window(GdkMonitor *monitor)
{
    GtkWindow *background_window = GTK_WINDOW(gtk_window_new(
        GTK_WINDOW_TOPLEVEL));
    gtk_window_set_type_hint(background_window, GDK_WINDOW_TYPE_HINT_DESKTOP);
    gtk_window_set_keep_below(background_window, TRUE);
    gtk_widget_set_name(GTK_WIDGET(background_window), "background");

    // Set Window Size to Monitor Size
    set_window_to_monitor_size(monitor, background_window);

    g_signal_connect(background_window, "realize", G_CALLBACK(hide_mouse_cursor),
                     NULL);
    // TODO: is this needed?
    g_signal_connect(background_window, "destroy", G_CALLBACK(gtk_main_quit),
                     NULL);

    return background_window;
}


/* Set the Window's Minimum Size to the Default Screen's Size */
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window)
{
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    gtk_widget_set_size_request(
        GTK_WIDGET(window),
        geometry.width,
        geometry.height
    );
    gtk_window_move(window, geometry.x, geometry.y);
    gtk_window_set_resizable(window, FALSE);
}


/* Hide the mouse cursor when it is hovered over the given widget.
 *
 * Note: This has no effect when used with a GtkEntry widget.
 */
static void hide_mouse_cursor(GtkWidget *widget, gpointer user_data)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *blank_cursor = gdk_cursor_new_for_display(display, GDK_BLANK_CURSOR);
    GdkWindow *window = gtk_widget_get_window(widget);
    if (window != NULL) {
        gdk_window_set_cursor(window, blank_cursor);
    }
}


/* Move the mouse cursor to the upper-left corner of the primary screen.
 *
 * This is necessary for hiding the mouse cursor because we cannot hide the
 * mouse cursor when it is hovered over the GtkEntry password input. Instead,
 * we hide the cursor when it is over the background windows and then move the
 * mouse to the corner of the screen where it should hover over the background
 * window or main window instead.
 */
static void move_mouse_to_background_window(void)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *mouse = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    GdkScreen *screen = gdk_display_get_default_screen(display);

    gdk_device_warp(mouse, screen, 0, 0);
}


/* Create & Configure the Main Window */
static void setup_main_window(Config *config, UI *ui)
{
    GtkWindow *main_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

    // gtk_container_set_border_width(GTK_CONTAINER(main_window), config->layout_spacing);
    gtk_widget_set_name(GTK_WIDGET(main_window), "main");

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor(display, 0);
    set_window_to_monitor_size(monitor, GTK_WINDOW(main_window));

    g_signal_connect(main_window, "show", G_CALLBACK(place_main_window), ui);
    g_signal_connect(main_window, "realize", G_CALLBACK(hide_mouse_cursor), NULL);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    ui->main_window = main_window;
}


/* Move the Main Window to the Center of the Primary Monitor
 *
 * This is done after the main window is shown(via the "show" signal) so that
 * the width of the window is properly calculated. Otherwise the returned size
 * will not include the size of the password label text.
 */
static void place_main_window(GtkWidget *main_window, gpointer user_data)
{
    // Get the Geometry of the Primary Monitor
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *primary_monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle primary_monitor_geometry;
    gdk_monitor_get_geometry(primary_monitor, &primary_monitor_geometry);

    // Get the Geometry of the Window
    gint window_width, window_height;
    gtk_window_get_size(GTK_WINDOW(main_window), &window_width, &window_height);

    gtk_window_move(
        GTK_WINDOW(main_window),
        primary_monitor_geometry.x + primary_monitor_geometry.width / 2 - window_width / 2,
        primary_monitor_geometry.y + primary_monitor_geometry.height / 2 - window_height / 2);

    if(user_data != NULL) {
        UI *ui = (UI*) user_data;
        gtk_stack_set_visible_child_full(ui->layout_stack, UI_STACK_LOGIN, GTK_STACK_TRANSITION_TYPE_UNDER_UP);
    }
}


/* Add a Stack for All Widgets */
static void create_and_attach_layout_stack(UI *ui)
{
    ui->layout_stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_duration(GTK_STACK(ui->layout_stack), 1000);

    gtk_container_add(GTK_CONTAINER(ui->main_window),
                    GTK_WIDGET(ui->layout_stack));

}

#define GAUSS_KERNEL_SIZE 17
static GdkPixbuf *blur_pixbuf(GdkPixbuf *buf, int radius)
{
    GdkPixbuf *dest = gdk_pixbuf_copy(buf);

    int width = gdk_pixbuf_get_width(buf);
    int height = gdk_pixbuf_get_height(buf);
    int stride = gdk_pixbuf_get_rowstride(buf);
    int num_channels = gdk_pixbuf_get_n_channels(buf);

    guchar *src_data = gdk_pixbuf_get_pixels(buf);
    guchar *dst_data = gdk_pixbuf_get_pixels(dest);

    g_assert(num_channels == 3);
    g_assert(gdk_pixbuf_get_bits_per_sample(buf) == 8);

    float sigma = (float)radius / 2.0f;
    sigma = MAX(sigma, 1.0f);

    // Kernel setup
    const int kernel_width = (2 * radius) + 1;
    float kernel[kernel_width][kernel_width];
    float kernel_sum = 0;

    // populate kernel
    for (int ky = -radius; ky < radius; ky++) {
        for (int kx = -radius; kx < radius; kx++) {
            float e_numerator = (float) -(kx*kx + ky*ky);
            float e_denominator = 2.0f * sigma*sigma;
            float e_term = expf(e_numerator / e_denominator);

            float kernel_value = e_term / (2.0f * M_PIf * sigma*sigma);
            kernel[ky + radius][kx + radius] = kernel_value;
            kernel_sum += kernel_value;
        }
    }
    // normalize kernel
    for (int ky = 0; ky < kernel_width; ky++) {
        for (int kx = 0; kx < kernel_width; kx++) {
            kernel[ky][kx] /= kernel_sum;
        }
    }

    // TODO: handle edges
    for (int y = radius; y < (height - radius); y++) {
        for (int x = radius; x < (width - radius); x++) {
            guchar *dst_pixel = dst_data + y * stride + x * num_channels;

            float r, g, b;
            r = g = b = 0.0f;

            for (int ky = -radius; ky < radius; ky++) {
                for (int kx = -radius; kx < radius; kx++) {
                    float kernel_value = kernel[ky + radius][kx + radius];
                    guchar *src_pixel = src_data + ((y + ky) * stride) + ((x + kx) * num_channels);

                    r += src_pixel[0] * kernel_value;
                    g += src_pixel[1] * kernel_value;
                    b += src_pixel[2] * kernel_value;
                }
            }

            dst_pixel[0] = (guchar) r;
            dst_pixel[1] = (guchar) g;
            dst_pixel[2] = (guchar) b;
        }
    }
    

   return dest;
}

/* Add a Layout Container for The login Widgets */
static void create_and_attach_layout_container(UI *ui)
{
    ui->layout_container = GTK_LAYOUT(gtk_layout_new(NULL, NULL));

    GdkPixbuf *buf = gdk_pixbuf_new_from_file_at_size("/home/david/Pictures/wallpaper.jpg", 800, 800, NULL);
    GdkPixbuf *blurred_buf = blur_pixbuf(buf, 15);
    g_object_unref(buf);

    ui->login_background = GTK_IMAGE(gtk_image_new_from_pixbuf(blurred_buf));
    gtk_container_add(GTK_CONTAINER(ui->layout_container),
                    GTK_WIDGET(ui->login_background));

    ui->login_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->login_container, 5);
    gtk_grid_set_row_spacing(ui->login_container, 5);

    gtk_container_add(GTK_CONTAINER(ui->layout_container),
                    GTK_WIDGET(ui->login_container));

    gtk_stack_add_named(GTK_STACK(ui->layout_stack),
                      GTK_WIDGET(ui->layout_container),
                      UI_STACK_LOGIN);
}

/* Add a Layout Container for overlay Widgets */
static void create_and_attach_overlay_container(UI *ui)
{
    ui->overlay_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->overlay_container, 5);
    gtk_grid_set_row_spacing(ui->overlay_container, 5);
    gtk_widget_set_name(GTK_WIDGET(ui->overlay_container), "overlay");

    gtk_stack_add_named(GTK_STACK(ui->layout_stack),
                      GTK_WIDGET(ui->overlay_container),
                      UI_STACK_OVERLAY);
}

/* Create a container for the system information & current time.
 *
 * Set the system information text by querying LightDM & the Config, but leave
 * the time blank & let the timer update it.
 */
static void create_and_attach_sys_info_label(Config *config, UI *ui)
{
    /*if (!config->show_sys_info) {
        return;
    }*/
    // container for system info & time
    ui->info_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->info_container, 0);
    gtk_grid_set_row_spacing(ui->info_container, 5);
    gtk_widget_set_name(GTK_WIDGET(ui->info_container), "info");

    // system info: <user>@<hostname>
    const gchar *hostname = lightdm_get_hostname();
    gchar *output_string;
    int output_string_length = asprintf(&output_string, "%s@%s",
                                        config->login_user, hostname);
    if (output_string_length >= 0) {
        ui->sys_info_label = gtk_label_new(output_string);
    } else {
        g_warning("Could not allocate memory for system info string.");
        ui->sys_info_label = gtk_label_new("");
    }
    gtk_label_set_xalign(GTK_LABEL(ui->sys_info_label), 0.0f);
    gtk_widget_set_name(GTK_WIDGET(ui->sys_info_label), "sys-info");

    // time: filled out by callback
    ui->time_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(ui->time_label), 1.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(ui->time_label), TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->time_label), "time-info");

    // attach labels to info container, attach info container to layout.
    gtk_grid_attach(
        ui->info_container, GTK_WIDGET(ui->sys_info_label), 0, 0, 1, 1);
    gtk_grid_attach(
        ui->info_container, GTK_WIDGET(ui->time_label), 1, 0, 1, 1);
    gtk_grid_attach(
        ui->login_container, GTK_WIDGET(ui->info_container), 0, 0, 2, 1);
}


/* Add a label & entry field for the user's password.
 *
 * If the `show_password_label` member of `config` is FALSE,
 * `ui->password_label` is left as NULL.
 */
static void create_and_attach_password_field(Config *config, UI *ui)
{
    ui->password_input = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ui->password_input), FALSE);
    if (config->password_char != NULL) {
        gtk_entry_set_invisible_char(GTK_ENTRY(ui->password_input), *config->password_char);
    }
    gtk_entry_set_alignment(GTK_ENTRY(ui->password_input),
                            config->password_alignment);
    // TODO: The width is usually a little shorter than we specify. Is there a
    // way to force this exact character width?
    // Maybe use 2 GtkBoxes instead of a GtkGrid?
    gtk_entry_set_width_chars(GTK_ENTRY(ui->password_input),
                              config->password_input_width);
    gtk_widget_set_name(GTK_WIDGET(ui->password_input), "password");
    const gint top = config->show_sys_info ? 1 : 0;
    gtk_grid_attach(ui->login_container, ui->password_input, 1, top, 1, 1);

    if (config->show_password_label) {
        ui->password_label = gtk_label_new(config->password_label_text);
        gtk_label_set_justify(GTK_LABEL(ui->password_label), GTK_JUSTIFY_RIGHT);
        gtk_grid_attach_next_to(ui->login_container, ui->password_label,
                                ui->password_input, GTK_POS_LEFT, 1, 1);
    }
}


/* Add a label for feedback to the user */
static void create_and_attach_feedback_label(UI *ui)
{
    ui->feedback_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ui->feedback_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_no_show_all(ui->feedback_label, TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->feedback_label), "error");

    GtkWidget *attachment_point;
    gint width;
    if (ui->password_label == NULL) {
        attachment_point = ui->password_input;
        width = 1;
    } else {
        attachment_point = ui->password_label;
        width = 2;
    }

    gtk_grid_attach_next_to(ui->login_container, ui->feedback_label,
                            attachment_point, GTK_POS_BOTTOM, width, 1);
}

/* Attach a style provider to the screen, using color options from config */
static void attach_config_colors_to_screen(Config *config)
{
    GtkCssProvider* provider = gtk_css_provider_new();

    GdkRGBA *caret_color;
    if (config->show_input_cursor) {
        caret_color = config->password_color;
    } else {
        caret_color = config->password_background_color;
    }

    char *css;
    int css_string_length = asprintf(&css,
        "* {\n"
            "font-family: %s;\n"
            "font-size: %s;\n"
            "font-weight: %s;\n"
            "font-style: %s;\n"
        "}\n"
        "label {\n"
            "color: %s;\n"
        "}\n"
        "label#error {\n"
            "color: %s;\n"
        "}\n"
        "#background {\n"
            "background-color: %s;\n"
        "}\n"
        "#background.with-image {\n"
            "background-image: image(url(%s), %s);\n"
            "background-repeat: no-repeat;\n"
            "background-size: %s;\n"
            "background-position: center;\n"
        "}\n"
        "#main, #password {\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "border-style: solid;\n"
        "}\n"
        "#main {\n"
            // "background-color: %s;\n"
        "}\n"
        "#overlay {\n"
            "background-color: #352a4a;\n"
            "background-image: url(\"/home/david/Pictures/wallpaper.jpg\");\n"
            "background-repeat: no-repeat;\n"
            "background-size: 100%%;\n"
            "background-position: center;\n"
        "}\n"
        "#password {\n"
            "color: %s;\n"
            "caret-color: %s;\n"
            "background-color: %s;\n"
            "border-width: %s;\n"
            "border-color: %s;\n"
            "border-radius: %s;\n"
            "background-image: none;\n"
            "box-shadow: none;\n"
            "border-image-width: 0;\n"
        "}\n"
        "#info {\n"
            "margin: %s;\n"
        "}\n"
        "#info label {\n"
            "font-family: %s;\n"
            "font-size: %s;\n"
            "color: %s;\n"
        "}\n"

        // *
        , config->font
        , config->font_size
        , config->font_weight
        , config->font_style
        // label
        , gdk_rgba_to_string(config->text_color)
        // label#error
        , gdk_rgba_to_string(config->error_color)
        // #background
        , gdk_rgba_to_string(config->background_color)
        // #background.image-background
        , config->background_image
        , gdk_rgba_to_string(config->background_color)
        , config->background_image_size
        // #main, #password
        , config->border_width
        , gdk_rgba_to_string(config->border_color)
        // #main
        // , gdk_rgba_to_string(config->window_color)
        // #password
        , gdk_rgba_to_string(config->password_color)
        , gdk_rgba_to_string(caret_color)
        , gdk_rgba_to_string(config->password_background_color)
        , config->password_border_width
        , gdk_rgba_to_string(config->password_border_color)
        , config->password_border_radius
        // #info
        , config->sys_info_margin
        // #info label
        , config->sys_info_font
        , config->sys_info_font_size
        , gdk_rgba_to_string(config->sys_info_color)
    );

    if (css_string_length >= 0) {
        gtk_css_provider_load_from_data(provider, css, -1, NULL);

        GdkScreen *screen = gdk_screen_get_default();
        gtk_style_context_add_provider_for_screen(
            screen, GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    }


    g_object_unref(provider);
}
