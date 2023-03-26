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
#include "network.h"
#include "battery.h"

#define UI_STACK_OVERLAY "overlay"
#define UI_STACK_LOGIN "login"


static UI *new_ui(Config *config);
static void setup_background_windows(Config *config, UI *ui);
static GtkWindow *new_background_window(GdkMonitor *monitor);
static void set_window_to_monitor_size(GdkMonitor *monitor, GtkWindow *window);
static void hide_mouse_cursor(GtkWidget *window, gpointer user_data);
static void show_default_cursor(GtkWidget *window, gpointer user_data);
// static void move_mouse_to_background_window(void);

static void setup_main_window(Config *config, UI *ui);
static void place_main_window(GtkWidget *main_window, gpointer user_data);
static void create_and_attach_layout_stack(UI *ui);
static void init_background_image(UI* ui, gchar* background_image);
static void create_and_attach_overlay_container(UI *ui);
static void create_and_attach_layout_container(UI *ui, gchar *background_image);
static void attach_config_colors_to_screen(Config *config);


/* Initialize the Main Window & it's Children */
UI *initialize_ui(Config *config)
{
    UI *ui = new_ui(config);

    // Setup Windows
    setup_background_windows(config, ui);
    // move_mouse_to_background_window();
    setup_main_window(config, ui);

    init_background_image(ui, config->background_image);
    create_and_attach_layout_stack(ui);

    create_and_attach_overlay_container(ui);
    create_and_attach_layout_container(ui, config->background_image);

    gtk_stack_set_visible_child_full(ui->layout_stack, UI_STACK_OVERLAY, GTK_STACK_TRANSITION_TYPE_OVER_DOWN);

    attach_config_colors_to_screen(config);

    return ui;
}

void ui_cover(UI* ui)
{
    gtk_stack_set_visible_child_full(ui->layout_stack, UI_STACK_OVERLAY, GTK_STACK_TRANSITION_TYPE_OVER_DOWN);
    gtk_widget_grab_focus(GTK_WIDGET(ui->overlay_container));
}

void ui_uncover(UI* ui)
{
    gtk_stack_set_visible_child_full(ui->layout_stack, UI_STACK_LOGIN, GTK_STACK_TRANSITION_TYPE_UNDER_UP);
    gtk_widget_grab_focus(ui->login_ui->password_input);
    gtk_entry_set_text(GTK_ENTRY(ui->login_ui->password_input), "");
}

/* Create a new UI with all values initialized to NULL */
static UI *new_ui(Config *config)
{
    UI *ui = malloc(sizeof(UI));
    if (ui == NULL) {
        g_error("Could not allocate memory for UI");
    }
    ui->background_windows = NULL;
    ui->monitor_count = 0;
    ui->main_window = NULL;

    ui->layout = NULL;
    ui->layout_vertical = NULL;
    ui->overlay_container = NULL;

    ui->login_ui = initialize_login_ui(config);

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
static void show_default_cursor(GtkWidget *widget, gpointer user_data)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkCursor *cursor = gdk_cursor_new_for_display(display, GDK_ARROW);
    GdkWindow *window = gtk_widget_get_window(widget);
    if (window != NULL) {
        gdk_window_set_cursor(window, cursor);
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
// static void move_mouse_to_background_window(void)
// {
//     GdkDisplay *display = gdk_display_get_default();
//     GdkDevice *mouse = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
//     GdkScreen *screen = gdk_display_get_default_screen(display);

//     gdk_device_warp(mouse, screen, 0, 0);
// }


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
    g_signal_connect(main_window, "realize", G_CALLBACK(show_default_cursor),
                     NULL);
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

    if(OVERLAY_DEBUG && user_data != NULL) {
        UI *ui = (UI*) user_data;
        ui_uncover(ui);
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

/**
 * Apply Gaussian Blur to a GdkPixBuf.
 * Edges are treated as if a mirrored image was off to each side.
*/
static void blur_pixbuf(GdkPixbuf *buf, int radius)
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
    const int kernel_width = (2*radius) + 1;
    float kernel[kernel_width];
    float kernel_sum = 0;

    // populate kernel
    for (int k = -radius; k < radius; k++) {
        float e_numerator = (float) -(k*k);
        float e_denominator = 2.0f * sigma*sigma;
        float e_term = expf(e_numerator / e_denominator);

        float kernel_value = e_term / (2.0f * M_PIf * sigma*sigma);
        kernel[k + radius] = kernel_value;
        kernel_sum += kernel_value;
    }
    // normalize kernel
    for (int k = 0; k < kernel_width; k++) {
        kernel[k] /= kernel_sum;
    }

    // Horizontal blur
    for (int y = 0; y < (height); y++) {
        for (int x = 0; x < (width); x++) {
            guchar *dst_pixel = dst_data + y * stride + x * num_channels;

            float r, g, b;
            r = g = b = 0.0f;

            for (int k = -radius; k < radius; k++) {
                int src_x = x + k;
                if (src_x < 0) {
                    src_x = -src_x + 1;
                } else if (src_x >= width) {
                    src_x = width - (width - src_x);
                }
                float kernel_value = kernel[k + radius];
                guchar *src_pixel = src_data + (y * stride) + (src_x * num_channels);

                r += src_pixel[0] * kernel_value;
                g += src_pixel[1] * kernel_value;
                b += src_pixel[2] * kernel_value;
            }

            dst_pixel[0] = (guchar) r;
            dst_pixel[1] = (guchar) g;
            dst_pixel[2] = (guchar) b;
        }
    }

    // vertical blur
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *dst_pixel = src_data + y * stride + x * num_channels;

            float r, g, b;
            r = g = b = 0.0f;

            for (int k = -radius; k < radius; k++) {
                int src_y = y + k;
                if (src_y < 0) {
                    src_y = -src_y + 1;
                } else if (src_y >= height) {
                    src_y = height - (height - src_y);
                }

                    float kernel_value = kernel[k + radius];
                    guchar *src_pixel = dst_data + (src_y * stride) + (x * num_channels);

                    r += src_pixel[0] * kernel_value;
                    g += src_pixel[1] * kernel_value;
                    b += src_pixel[2] * kernel_value;
            }

            dst_pixel[0] = (guchar) r;
            dst_pixel[1] = (guchar) g;
            dst_pixel[2] = (guchar) b;
        }
    }
    
   g_object_unref(dest);
}

static gboolean draw_overlay_background(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    struct BackgroundPixbuf* bg = (struct BackgroundPixbuf*) data;
    gdk_cairo_set_source_pixbuf(cr, bg->buf, bg->x, bg->y);
    cairo_paint(cr);

    // overlay the gradient
    GtkAllocation rect = {0};
    gtk_widget_get_allocation(widget, &rect);

    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, 0, 0, rect.height);

    cairo_pattern_add_color_stop_rgba(gradient, 0, 0, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(gradient, 0.5, 0, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(gradient, 0.8, 0, 0, 0, 0.4);

    cairo_rectangle(cr, 0, 0, rect.width, rect.height);
    cairo_set_source(cr, gradient);
    cairo_fill(cr);

    cairo_pattern_destroy(gradient);

    return FALSE;
}

static gboolean draw_blurred_background(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    struct BackgroundPixbuf* bg = (struct BackgroundPixbuf*) data;
    gdk_cairo_set_source_pixbuf(cr, bg->buf, bg->x, bg->y);
    cairo_paint(cr);

    cairo_set_source_rgba(cr, 0, 0, 0, 0.4);

    cairo_paint(cr);

    return FALSE;
}

static void init_background_image(UI* ui, gchar* background_image)
{
    char *bg_url = strndup(background_image + 1, strlen(background_image) - 2);
    if (strlen(bg_url) > 0) {
        fprintf(stderr, "[GREETER] %s\n", bg_url);
        int window_width, window_height;
        gtk_window_get_size(ui->main_window, &window_width, &window_height);

        GdkPixbuf* buf = load_image_to_cover(bg_url, (guint) window_width, (guint) window_height, NULL);
        // Offset to center the image
        gdouble bg_x_offset = -((gdk_pixbuf_get_width(buf) / 2) - (window_width / 2));
        gdouble bg_y_offset = -((gdk_pixbuf_get_height(buf) / 2) - (window_height / 2));

        // Setup for drawing the picture on the overlay
        ui->overlay_bg = malloc(sizeof(struct BackgroundPixbuf));
        ui->overlay_bg->buf = buf;
        ui->overlay_bg->x = bg_x_offset;
        ui->overlay_bg->y = bg_y_offset;
        
        // Blurred Background
        GdkPixbuf *blurred_buf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, window_width, window_height);
        gdk_pixbuf_copy_area(buf, (int)-bg_x_offset, (int)-bg_y_offset, window_width, window_height, blurred_buf, 0, 0);
        blur_pixbuf(blurred_buf, 25);

        ui->login_bg = malloc(sizeof(struct BackgroundPixbuf));
        ui->login_bg->buf = blurred_buf;
        ui->login_bg->x = 0;
        ui->login_bg->y = 0;

    }
    free(bg_url);
}

/* Add a Layout Container for The login Widgets */
static void create_and_attach_layout_container(UI *ui, gchar *background_image)
{
    ui->layout = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
    ui->layout_vertical = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
    gtk_widget_set_name(GTK_WIDGET(ui->layout_vertical), "layout-box");
    
    // set_main_background(ui, background_image);
    g_signal_connect(G_OBJECT(ui->layout), "draw", G_CALLBACK(draw_blurred_background), ui->login_bg);

    gtk_box_set_center_widget(GTK_BOX(ui->layout_vertical),
                            GTK_WIDGET(ui->login_ui->login_container));

    gtk_box_set_center_widget(GTK_BOX(ui->layout),
                            GTK_WIDGET(ui->layout_vertical));

    gtk_stack_add_named(GTK_STACK(ui->layout_stack),
                      GTK_WIDGET(ui->layout),
                      UI_STACK_LOGIN);
}

/* Add a Layout Container for overlay Widgets */
static void create_and_attach_overlay_container(UI *ui)
{
    ui->overlay_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->overlay_container, 5);
    gtk_grid_set_row_spacing(ui->overlay_container, 5);
    gtk_widget_set_name(GTK_WIDGET(ui->overlay_container), "overlay");

    g_signal_connect(G_OBJECT(ui->overlay_container), "draw", G_CALLBACK(draw_overlay_background), ui->overlay_bg);

    // time: filled out by callback
    ui->time_label = gtk_label_new("Time");
    gtk_widget_set_vexpand(GTK_WIDGET(ui->time_label), TRUE);

    gtk_label_set_yalign(GTK_LABEL(ui->time_label), 1.0f);
    gtk_label_set_xalign(GTK_LABEL(ui->time_label), 1.0f);
    gtk_widget_set_name(GTK_WIDGET(ui->time_label), "time-info");

    ui->date_label = gtk_label_new("Date");
    gtk_widget_set_name(GTK_WIDGET(ui->date_label), "date-info");
    gtk_label_set_xalign(GTK_LABEL(ui->date_label), 1.0f);

    gtk_grid_attach(
        ui->overlay_container, GTK_WIDGET(ui->time_label), 0, 0, 1, 1);
    gtk_grid_attach(
        ui->overlay_container, GTK_WIDGET(ui->date_label), 0, 1, 1, 1);
    
    // battery widget
    ui->battery_display = battery_widget();
    gtk_widget_set_hexpand(GTK_WIDGET(ui->battery_display), TRUE);

    // network widget
    ui->network_display = init_network_widget();

    gtk_grid_attach(
        ui->overlay_container, GTK_WIDGET(ui->battery_display), 2, 1, 1, 1);
    gtk_grid_attach(
        ui->overlay_container, GTK_WIDGET(ui->network_display), 3, 1, 1, 1);

    gtk_grid_insert_column(ui->overlay_container, 2);

    gtk_stack_add_named(GTK_STACK(ui->layout_stack),
                      GTK_WIDGET(ui->overlay_container),
                      UI_STACK_OVERLAY);
}

/* Attach a style provider to the screen, using color options from config */
static void attach_config_colors_to_screen(Config* config)
{
    GtkCssProvider* provider = gtk_css_provider_new();

    GdkRGBA* caret_color;
    if (config->show_input_cursor) {
        caret_color = config->password_color;
    } else {
        caret_color = config->password_background_color;
    }

    char* css;
    int css_string_length = asprintf(&css,
        "* {\n"
            "font-family: %s;\n"
            "font-family: \"Ubuntu\", \"Sans\";\n"
            "font-weight: 500;"
            "font-size: %s;\n"
            "color: #f1f1f1;\n"
        "}\n"
        "#time-info {\n"
            "padding-left: 1rem;\n"
            "font-size: 8em;\n"
            "font-weight: 300;"
        "\n}"
        "#date-info {\n"
            "padding-right: 0.7rem;\n"
            "font-size: 2em;\n"
            "font-weight: 200;"
        "\n}"
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
            "padding: 3em 2em;\n"
        "}\n"
        "#password {\n"
            "color: %s;\n"
            "caret-color: %s;\n"
            "background-color: %s;\n"
            "border-width: %s;\n"
            "border-width: 0.1rem;\n"
            "border-color: %s;\n"
            "border-radius: 0;\n"
            "background-image: none;\n"
            "box-shadow: none;\n"
            "border-image-width: 0;\n"
        "}\n"
        "#login-button {\n"
            "border-radius: 0px;\n"
            "color: white;\n"
            "background: #f1f1f1;\n"
            "border-color: #f1f1f1;\n"
            "border-width: 0.1rem;\n"
        "}\n"
        "#login-button:hover {\n"
            "background: #c7d1d1;\n"
        "}\n"
        "#current-user-image {\n"
            "border-radius: 100%%;\n"
            "padding: 1em;\n"
            "border: 0.1em solid #f1f1f1;\n"
            "background-color: #1b1d1e;\n"
        "}\n"
        "#current-user {\n"
            "font-family: \"Ubuntu\", %s;\n"
            "font-size: 1.5em;\n"
            "color: %s;\n"
        "}\n"

        // *
        , config->font
        , config->font_size
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
        // #overlay
        //, gdk_rgba_to_string(config->background_color)
        // , config->background_image
        // #password
        , gdk_rgba_to_string(config->password_color)
        , gdk_rgba_to_string(caret_color)
        , gdk_rgba_to_string(config->password_background_color)
        , config->password_border_width
        , gdk_rgba_to_string(config->password_border_color)
        // #info label
        , config->sys_info_font
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
