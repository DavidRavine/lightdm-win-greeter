#define _GNU_SOURCE
#include "ui_login.h"
#include <lightdm.h>
#include <stdio.h>


static LoginUI *new_login_ui(void);
static void create_login_container(LoginUI *ui);
static void create_and_attach_sys_info_label(Config *config, LoginUI *ui);
static void create_and_attach_password_field(Config *config, LoginUI *ui);
static void create_and_attach_feedback_label(LoginUI *ui);

LoginUI *initialize_login_ui(Config *config)
{
    LoginUI *ui = new_login_ui();

    create_login_container(ui);

    create_and_attach_sys_info_label(config, ui);
    create_and_attach_password_field(config, ui);
    create_and_attach_feedback_label(ui);

    return ui;
}

/* Create the container for the login to live in */
static void create_login_container(LoginUI *ui)
{
    ui->login_container = GTK_GRID(gtk_grid_new());
    gtk_grid_set_column_spacing(ui->login_container, 5);
    gtk_grid_set_row_spacing(ui->login_container, 5);
}

/* Create a container for the system information & current time.
 *
 * Set the system information text by querying LightDM & the Config, but leave
 * the time blank & let the timer update it.
 */
static void create_and_attach_sys_info_label(Config *config, LoginUI *ui)
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


    // attach labels to info container, attach info container to layout.
    gtk_grid_attach(
        ui->info_container, GTK_WIDGET(ui->sys_info_label), 0, 0, 1, 1);

    gtk_grid_attach(
        ui->login_container, GTK_WIDGET(ui->info_container), 0, 0, 2, 1);
}


/* Add a label & entry field for the user's password.
 *
 * If the `show_password_label` member of `config` is FALSE,
 * `ui->password_label` is left as NULL.
 */
static void create_and_attach_password_field(Config *config, LoginUI *ui)
{
    ui->password_input = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(ui->password_input), FALSE);
    if (config->password_char != NULL) {
        gtk_entry_set_invisible_char(GTK_ENTRY(ui->password_input), *config->password_char);
    }
    gtk_entry_set_alignment(GTK_ENTRY(ui->password_input), 0);
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
static void create_and_attach_feedback_label(LoginUI *ui)
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

/* Create a new UI with all values initialized to NULL */
static LoginUI *new_login_ui(void)
{
    LoginUI *ui = malloc(sizeof(LoginUI));
    if (ui == NULL) {
        g_error("Could not allocate memory for LoginUI");
    }
    ui->password_label = NULL;
    ui->password_input = NULL;
    ui->feedback_label = NULL;

    return ui;
}
