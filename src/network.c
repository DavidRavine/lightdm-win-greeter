#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/wireless.h>
#include <stdlib.h>
#include <stdio.h>

#include "network.h"


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
        
        fprintf(stderr, "[GREETER] found network: %s\n", address->ifa_name);
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

/* Create an icon image representing the currently connected network */
GtkWidget* init_network_widget(void)
{
    GdkPixbuf* icon;
    enum NetworkType type = get_network_type();

    if (type == NW_WIRED) {
        icon = gtk_icon_theme_load_icon(
            gtk_icon_theme_get_default(),
            "network-wired",
            20,
            GTK_ICON_LOOKUP_FORCE_SYMBOLIC,
            NULL);
    } else if (type == NW_WIRELESS) {
        icon = gtk_icon_theme_load_icon(
            gtk_icon_theme_get_default(),
            "network-wireless",
            20,
            GTK_ICON_LOOKUP_FORCE_SYMBOLIC,
            NULL);
    } else {
        icon = gtk_icon_theme_load_icon(
            gtk_icon_theme_get_default(),
            "network-offline",
            20,
            GTK_ICON_LOOKUP_FORCE_SYMBOLIC,
            NULL);
    }

    if (!icon) {
        fprintf(stderr, "[GREETER] network icon not found!\n");
    }

    GtkImage* icon_image = GTK_IMAGE(gtk_image_new_from_pixbuf(icon));
    g_object_unref(icon);

    return GTK_WIDGET(icon_image);
}
