#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/wireless.h>
#include <stdlib.h>
#include <stdio.h>

#include "network.h"


static GdkPixbuf* icon_wireless(int size);
static GdkPixbuf* icon_ethernet(int size);
static GdkPixbuf* icon_offline(int size);

enum NetworkType {
    NW_NONE,
    NW_WIRED,
    NW_WIRELESS,
};

static gboolean is_wireless(char* if_name)
{
    int sock = -1;
    struct iwreq pwrq = { 0 };

    strncpy(pwrq.ifr_name, if_name, IFNAMSIZ);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        return FALSE;
    }

    if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
        close(sock);
        return TRUE;
    }

    close(sock);
    return FALSE;
}

static enum NetworkType get_network_type(void)
{
    struct ifaddrs *all_addresses, *address;
    all_addresses = NULL;

    if (getifaddrs(&all_addresses) == -1) {
        return NW_NONE;
    }
    if (all_addresses == NULL) {
        return NW_NONE;
    }

    gboolean has_wireles = FALSE;
    gboolean has_wired = FALSE;
    for(address = all_addresses; address != NULL; address = address->ifa_next) {
        if (strcmp(address->ifa_name, "lo") == 0)
            continue;
        
        if (address->ifa_addr == NULL || address->ifa_addr->sa_family != AF_PACKET)
            continue;
        
        if (is_wireless(address->ifa_name)) {
            has_wireles = TRUE;
        } else {
            has_wired = TRUE;
        }
    }

    freeifaddrs(all_addresses);

    if  (has_wired) {
        return NW_WIRED;
    } else if (has_wireles) {
        return NW_WIRELESS;
    } else {
        return NW_NONE;
    }
}

struct NetworkWidget {
    GtkWidget* image;
    enum NetworkType current_network;
};

static gboolean update_network_widget(struct NetworkWidget* nw_widget)
{
    enum NetworkType new_network = get_network_type();
    if (new_network != nw_widget->current_network) {
        nw_widget->current_network = new_network;

        GdkPixbuf* icon;
        if (new_network == NW_WIRED) {
            icon = icon_ethernet(27);

        } else if (new_network == NW_WIRELESS) {
            icon = icon_wireless(27);
        } else {
            icon = icon_offline(27);
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(nw_widget->image), icon);
        g_object_unref(icon);
    }
    return TRUE;
}
/* Create an icon image representing the currently connected network */
GtkWidget* init_network_widget(void)
{
    GdkPixbuf* icon;
    enum NetworkType type = get_network_type();

    if (type == NW_WIRED) {
        icon = icon_ethernet(27);

    } else if (type == NW_WIRELESS) {
        icon = icon_wireless(27);
    } else {
        icon = icon_offline(27);
    }

    if (!icon) {
        g_warning("[GREETER] network icon not found!\n");
    }

    GtkImage* icon_image = GTK_IMAGE(gtk_image_new_from_pixbuf(icon));
    g_object_unref(icon);

    struct NetworkWidget* info = malloc(sizeof(struct NetworkWidget));
    info->current_network = type;
    info->image = GTK_WIDGET(icon_image);

    g_timeout_add_seconds(15, G_SOURCE_FUNC(update_network_widget), info);

    return GTK_WIDGET(icon_image);
}

static GdkPixbuf* icon_ethernet(int size)
{
    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double line_width = size / 10;
    const double top = line_width;
    const double left = line_width;
    const double right = size - line_width;
    const double base = size - line_width;
    const double monitor_bottom = size * 0.65;
    const double plug_bottom = monitor_bottom - (2 * line_width);
    const double plug_center = left + 2.2*line_width; 
    const double plug_right = plug_center + 2.2*line_width;

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, line_width);

    // common outline
    cairo_move_to(cr, plug_right, top);
    cairo_line_to(cr, plug_right, plug_bottom);
    cairo_line_to(cr, left, plug_bottom);
    cairo_line_to(cr, left, top);
    cairo_line_to(cr, right, top);
    cairo_line_to(cr, right, monitor_bottom);
    cairo_line_to(cr, plug_center, monitor_bottom);
    cairo_stroke(cr);

    // chord
    cairo_move_to(cr, plug_center, plug_bottom);
    cairo_line_to(cr, plug_center, base);
    cairo_stroke(cr);

    // plug details
    cairo_move_to(cr, plug_center, top + 1.6*line_width);
    cairo_line_to(cr, plug_center, top + 4*line_width);

    cairo_move_to(cr, left, top + 2.2*line_width);
    cairo_line_to(cr, plug_right, top + 2.2*line_width);
    cairo_stroke(cr);
    
    // Monitor base
    const double monitor_center = (right-plug_center)/2 + plug_center;
    cairo_move_to(cr, monitor_center, monitor_bottom);
    cairo_line_to(cr, monitor_center, base);

    cairo_move_to(cr, monitor_center - (2.5*line_width), base - (line_width/2));
    cairo_line_to(cr, monitor_center + (2.5*line_width), base - (line_width/2));
    cairo_stroke(cr);

    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}

static GdkPixbuf* icon_wireless(int size)
{
    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double line_width = size / 10;
    const double root_x = size / 2;
    const double root_y = size - (line_width * 3);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, line_width);

    cairo_arc(cr, root_x, root_y, line_width, 0, 2 * G_PI);
    cairo_fill(cr);

    cairo_arc(cr, root_x, root_y, 3*line_width, -3 * G_PI_4, -G_PI_4);
    cairo_stroke(cr);

    cairo_arc(cr, root_x, root_y, 5*line_width,  -3 * G_PI_4, -G_PI_4);
    cairo_stroke(cr);

    cairo_arc(cr, root_x, root_y, 7*line_width,  -3 * G_PI_4, -G_PI_4);
    cairo_stroke(cr);

    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}

static GdkPixbuf* icon_offline(int size)
{
    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double line_width = size / 10;
    const double center = size / 2;
    const double sign_center_x = size / 3;
    const double sign_center_y = size - sign_center_x;
    const double sign_radius = size / 3 - line_width;

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // internet globe
    cairo_arc(cr, center, center, center - line_width/2, -G_PI * 1.1, G_PI_2);
    cairo_stroke(cr);

    // inner lines "vertical"
    cairo_arc(cr, size+line_width/2, center, size*0.75, G_PI*1.05, G_PI*1.25);
    cairo_arc(cr, -line_width/2, center, size*0.75, -G_PI_4, G_PI_4*0.7);
    cairo_stroke(cr);

    // inner lines "horizontal"
    cairo_arc(cr, center, -4*line_width, size*0.75, G_PI_4*1.25, G_PI_2*0.8);
    cairo_arc(cr, center, -4*line_width, size*0.75, G_PI_2*1.2, G_PI*0.69);
    cairo_stroke(cr);

    cairo_arc(cr, center, -line_width, size*0.75, G_PI_4*1.3, G_PI_2*0.9);
    cairo_stroke(cr);

    // sign
    cairo_arc(cr, sign_center_x, sign_center_y, sign_radius, -G_PI_4, G_PI - G_PI_4);
    cairo_line_to(cr, sign_center_x, sign_center_y);
    cairo_arc(cr, sign_center_x, sign_center_y, sign_radius, G_PI - G_PI_4, -G_PI_4);
    cairo_line_to(cr, sign_center_x, sign_center_y);
    
    cairo_stroke(cr);


    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}
