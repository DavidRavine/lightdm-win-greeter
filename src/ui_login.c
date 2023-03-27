#define _GNU_SOURCE
#include "ui_login.h"
#include "utils.h"
#include <lightdm.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>


static LoginUI* new_login_ui(void);
static void create_login_container(LoginUI* ui);
static void load_and_attach_user_image(LoginUI* ui, Config* config);
static void create_and_attach_username_label(Config* config, LoginUI* ui);
static void create_and_attach_password_field(Config* config, LoginUI* ui);
static void create_and_attach_feedback_label(LoginUI* ui);
static char* user_get_pretty_name(const char* username);

LoginUI* initialize_login_ui(Config *config)
{
    LoginUI* ui = new_login_ui();

    create_login_container(ui);
    load_and_attach_user_image(ui, config);

    create_and_attach_username_label(config, ui);
    create_and_attach_password_field(config, ui);
    create_and_attach_feedback_label(ui);

    return ui;
}

/* Create the container for the login to live in */
static void create_login_container(LoginUI* ui)
{
    ui->login_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
}

/* Create a container for the system information & current time.
 *
 * Set the system information text by querying LightDM & the Config, but leave
 * the time blank & let the timer update it.
 */
static void create_and_attach_username_label(Config* config, LoginUI* ui)
{
    char* username = user_get_pretty_name(config->login_user);
    ui->username_label = gtk_label_new(username);
    free(username);

    gtk_label_set_xalign(GTK_LABEL(ui->username_label), 0.5f);
    gtk_widget_set_name(GTK_WIDGET(ui->username_label), "current-user");

    gtk_container_add(GTK_CONTAINER(ui->login_container),
                    GTK_WIDGET(ui->username_label));
}


/* Add a label & entry field for the user's password.
 *
 * If the `show_password_label` member of `config` is FALSE,
 * `ui->password_label` is left as NULL.
 */
static void create_and_attach_password_field(Config* config, LoginUI* ui)
{
    // Create the container
    ui->password_line = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

    // Create the input-field
    ui->password_input = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ui->password_input), FALSE);
    if (config->password_char != NULL) {
        gtk_entry_set_invisible_char(GTK_ENTRY(ui->password_input), *config->password_char);
    }
    gtk_entry_set_alignment(GTK_ENTRY(ui->password_input), 0);
    gtk_entry_set_placeholder_text(GTK_ENTRY(ui->password_input), "Password");

    gtk_widget_set_name(GTK_WIDGET(ui->password_input), "password");

    gtk_container_add(GTK_CONTAINER(ui->password_line),
                    GTK_WIDGET(ui->password_input));
    
    // Create the arrow-button
    ui->login_button = GTK_BUTTON(gtk_button_new());

    gtk_widget_set_name(GTK_WIDGET(ui->login_button), "login-button");
    gtk_button_set_relief(GTK_BUTTON(ui->login_button), GTK_RELIEF_NONE);
    GdkPixbuf* icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                            "go-next",
                            15,
                            GTK_ICON_LOOKUP_FORCE_SYMBOLIC,
                            NULL);
    if (!icon) {
        g_warning("[GREETER] icon not found!\n");
    }

    GtkImage* icon_image = GTK_IMAGE(gtk_image_new_from_pixbuf(icon));
    g_object_unref(icon);

    gtk_button_set_image(GTK_BUTTON(ui->login_button), GTK_WIDGET(icon_image));
    
    gtk_container_add(GTK_CONTAINER(ui->password_line),
                    GTK_WIDGET(ui->login_button));

    gtk_container_add(GTK_CONTAINER(ui->login_container),
                    GTK_WIDGET(ui->password_line));

}


/* Add a label for feedback to the user */
static void create_and_attach_feedback_label(LoginUI *ui)
{
    ui->feedback_label = gtk_label_new(" ");
    gtk_label_set_justify(GTK_LABEL(ui->feedback_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(ui->feedback_label), TRUE);
    gtk_widget_set_no_show_all(ui->feedback_label, TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->feedback_label), "error");

    gtk_container_add(GTK_CONTAINER(ui->login_container),
                    GTK_WIDGET(ui->feedback_label));
}

static GdkPixbuf* round_user_image(GdkPixbuf* source, int size)
{
    const double tau = 2 * G_PI;
    
    const int center = size / 2;
    const int source_width = gdk_pixbuf_get_width(source);
    const int source_height = gdk_pixbuf_get_height(source);

    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);
    
    // draw the background color
    cairo_set_source_rgb(cr, 0.106, 0.113, 0.117);
    cairo_arc(cr, center, center, center, 0, tau);
    cairo_fill(cr);

    // draw the image
    gdk_cairo_set_source_pixbuf(cr, source, center - (source_width / 2), center - (source_height / 2));

    cairo_arc(cr, center, center, center, 0, tau);
    cairo_fill(cr);

    cairo_arc(cr, center, center, center - 0.6, 0, tau);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, 1.2);

    cairo_stroke(cr);

    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}

static void load_and_attach_user_image(LoginUI* ui, Config* config)
{
    // User face
    GdkPixbuf* image;
    GError* error = NULL;

    gchar user_face[100] = {0};
    int printed = 0;
    printed = snprintf(user_face, 100, "/home/%s/.face", config->login_user);
    if (printed) {
        image = load_image_to_cover(user_face, 130, 130, &error);
    }

    if (error != NULL) {
        g_warning("[GREETER] error loading image %s\n", error->message);
    }

    if (!printed || error != NULL) {
        error = NULL;
        printed = snprintf(user_face, 100, "/usr/share/lightdm/users/%s", config->login_user);
        if (printed) {
            image = load_image_to_cover(user_face, 130, 130, &error);
        }
    }
    if (error != NULL) {
        g_warning("[GREETER] error loading image %s\n", error->message);
    }
    // Default image
    if (!printed || error != NULL) {
        error = NULL;
        image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                        "avatar-default",
                                        100,
                                        GTK_ICON_LOOKUP_FORCE_SIZE,
                                        &error);
    }
    if (error != NULL) {
        g_warning("[GREETER] icon 'avatar-default' not found: %s\n", error->message);
        image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE,  8, 130, 130);
    }

    GdkPixbuf* framed_image = round_user_image(image, 130);
    ui->user_image = GTK_IMAGE(gtk_image_new_from_pixbuf(framed_image));
    gtk_widget_set_halign(GTK_WIDGET(ui->user_image), GTK_ALIGN_CENTER);
    g_object_unref(G_OBJECT(image));

    gtk_container_add(GTK_CONTAINER(ui->login_container),
                    GTK_WIDGET(ui->user_image));
}

/* Create a new UI with all values initialized to NULL */
static LoginUI* new_login_ui(void)
{
    LoginUI* ui = malloc(sizeof(LoginUI));
    if (ui == NULL) {
        g_error("Could not allocate memory for LoginUI");
    }
    ui->password_input = NULL;
    ui->feedback_label = NULL;

    return ui;
}

static char* user_get_pretty_name(const char* username)
{

    int file_descriptor = open("/etc/passwd", O_RDONLY);
    if (file_descriptor < 0)
        return strdup(username);

    struct stat passwd_stat;
    int stat_error = fstat(file_descriptor, &passwd_stat);
    if (stat_error < 0) {
        return strdup(username);
    }
    char* contents = mmap(NULL, (size_t) passwd_stat.st_size,
        PROT_READ, MAP_PRIVATE,
        file_descriptor, 0);
    
    char* read_head = contents;
    size_t username_length = strlen(username);

    char* current_user = strndup(read_head, username_length);
    char* pretty_name = strdup(username);

    while ((read_head - contents) < passwd_stat.st_size - 1) {
        if (strcmp(username, current_user) == 0) {
            // find name
            for (int col = 0; col < 4; col++) {
                read_head += strcspn(read_head, ":") + 1;
            }
            size_t name_length = strcspn(read_head, ",");
            if (name_length > 0)
                pretty_name = strndup(read_head, name_length);
            break;
        }
        size_t line_length = strcspn(read_head, "\n") + 1;
        if ((read_head + line_length) >= (contents + passwd_stat.st_size-1)) {
            break;
        }
        read_head += line_length;
        strncpy(current_user, read_head, username_length);
    }

    munmap(contents, (size_t) passwd_stat.st_size);
    free(current_user);

    return pretty_name;
    
}