#define _GNU_SOURCE
#include "ui_login.h"
#include <lightdm.h>
#include <stdio.h>


static LoginUI* new_login_ui(void);
static void create_login_container(LoginUI* ui);
static void create_and_attach_username_label(Config* config, LoginUI* ui);
static void create_and_attach_password_field(Config* config, LoginUI* ui);
static void create_and_attach_feedback_label(LoginUI* ui);

LoginUI* initialize_login_ui(Config *config)
{
    LoginUI* ui = new_login_ui();

    create_login_container(ui);

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
    // system info: <user>@<hostname>
    ui->username_label = gtk_label_new(config->login_user);

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
        fprintf(stderr, "[GREETER] icon not found!\n");
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
    ui->feedback_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(ui->feedback_label), GTK_JUSTIFY_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(ui->feedback_label), TRUE);
    gtk_widget_set_no_show_all(ui->feedback_label, TRUE);
    gtk_widget_set_name(GTK_WIDGET(ui->feedback_label), "error");

    gtk_container_add(GTK_CONTAINER(ui->login_container),
                    GTK_WIDGET(ui->feedback_label));
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
