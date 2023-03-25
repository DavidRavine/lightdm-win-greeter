#include "battery.h"

#include <upower.h>

enum PowerType {
    POWER_LINE,
    POWER_BATTERY,
    POWER_BATTERY_CHARGING,
};

struct PowerStats {
    enum PowerType type;
    double charge;
};

static gboolean power_devices(struct PowerStats* stats)
{
    GError* error = NULL;
    UpClient* client = up_client_new_full(NULL, &error);
    if  (error != NULL) {
        g_warning("[GREETER] could not get UPower client: %s\n", error->message);
        return FALSE;
    }

    GPtrArray* devices = up_client_get_devices2(client);
    if (devices == NULL) return FALSE;

    gboolean has_line_power = FALSE;
    gboolean has_battery = FALSE;
    double charge_percent = 100;
    for(guint i = 0; i < devices->len; i++) {
        UpDevice* device = (UpDevice*) devices->pdata[i];
        guint kind;
        g_object_get(G_OBJECT(device), "kind", &kind, NULL);

        fprintf(stderr, "[GREETER] device kind = %d\n", kind);
        
        UpDeviceKind device_kind = kind;
        if (device_kind == UP_DEVICE_KIND_LINE_POWER) {
            has_line_power = TRUE;
        } else if (device_kind == UP_DEVICE_KIND_BATTERY) {
            has_battery = TRUE;
            g_object_get(G_OBJECT(device), "percentage", &charge_percent, NULL);
        }
    }

    g_ptr_array_unref(devices);

    enum PowerType type = POWER_LINE;
    if (has_battery) {
        type = has_line_power ? POWER_BATTERY_CHARGING : POWER_BATTERY;
    }

    stats->type = type;
    stats->charge = charge_percent;
    return TRUE;
}

GtkWidget* battery_widget(void)
{
    struct PowerStats stats;
    gboolean success = power_devices(&stats);

    const char* kind_str = "???";
    if (success) {
        if  (stats.type == POWER_BATTERY) {
            kind_str = "Bat";
        } else if (stats.type == POWER_BATTERY_CHARGING) {
            kind_str = "Bat(Chrg)";
        } else {
            kind_str = "Lin";
        }
    }

    char label[30] = { 0 };
    snprintf(label, 30, "%s : %.1f%%", kind_str, stats.charge);
    return gtk_label_new(label);
}
