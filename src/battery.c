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

struct BatteryWidgetInfo {
    GdkPixbuf* outline;
    GdkPixbuf* charger;
    double battery_level;
    gboolean is_charging;
    GtkWidget* widget;
};


static gboolean power_devices(struct PowerStats* stats);
static gboolean update_battery_status(struct BatteryWidgetInfo* info);


static gboolean power_devices(struct PowerStats* stats)
{
    GError* error = NULL;
    UpClient* client = up_client_new_full(NULL, &error);
    if  (error != NULL) {
        g_warning("Could not get UPower client: %s\n", error->message);
        return FALSE;
    }

    GPtrArray* devices = up_client_get_devices2(client);
    if (devices == NULL) return FALSE;

    gboolean is_charging = FALSE;
    gboolean has_battery = FALSE;

    double charge_percent = 100;
    for(guint i = 0; i < devices->len; i++) {
        UpDevice* device = (UpDevice*) devices->pdata[i];
        guint kind;
        g_object_get(G_OBJECT(device), "kind", &kind, NULL);

        UpDeviceKind device_kind = kind;
        if (device_kind == UP_DEVICE_KIND_BATTERY) {
            guint state;
            g_object_get(G_OBJECT(device), "state", &state, NULL);
            UpDeviceState device_state = state;
            if (device_state == UP_DEVICE_STATE_CHARGING) {
                is_charging = TRUE;
            }
            has_battery = TRUE;
            g_object_get(G_OBJECT(device), "percentage", &charge_percent, NULL);
        }
    }

    g_ptr_array_unref(devices);

    enum PowerType type = POWER_LINE;
    if (has_battery) {
        type = is_charging ? POWER_BATTERY_CHARGING : POWER_BATTERY;
    }

    stats->type = type;
    stats->charge = charge_percent;
    return TRUE;
}

static gboolean draw_battery_widget(GtkWidget* widget, cairo_t *cr, struct BatteryWidgetInfo* widget_info)
{
    const int size = gdk_pixbuf_get_width(widget_info->outline);

    const double top = size / 4;
    const double left = size * 0.28;
    const double bottom = size - (top / 2);
    const double right = size - left;
    const double width = right - left;
    const double height = bottom - top;
    const double line_width = height / 10;

    // draw outline
    gdk_cairo_set_source_pixbuf(cr, widget_info->outline, 0, 0);

    if (widget_info->is_charging) {
        // set mask
        cairo_rectangle(cr, 0, 0, size, top + (height / 3));
        cairo_fill(cr);
        cairo_rectangle(cr,
            left + line_width, top + (height / 3),
            size, size);
        cairo_fill(cr);

        // draw charger
        gdk_cairo_set_source_pixbuf(cr, widget_info->charger, 0, 0);
        cairo_paint(cr);

    } else {
        cairo_paint(cr);
    }

    // draw charge
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 
        left + line_width, bottom - line_width,
        width - (2*line_width), -(height - (2*line_width)) * widget_info->battery_level / 100);
    cairo_fill(cr);


    return FALSE;
}

static GdkPixbuf* init_charger(int size)
{
    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double top = size / 4;
    const double left = size * 0.28;
    const double bottom = size - (top / 2);
    const double height = bottom - top;
    const double line_width = height / 10;

    const double charger_left = left / 4 * 3;

    // draw charger
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, charger_left, top + (height * 0.7));
    cairo_line_to(cr, charger_left, size);
    cairo_stroke(cr);

    cairo_rectangle(cr,
        charger_left - (charger_left/2), top + (height/3*2),
        line_width*3, height / 5);
    cairo_fill(cr);

    // prongs
    cairo_set_line_width(cr, line_width * 0.9);
    cairo_move_to(cr, charger_left - (line_width/3*2), top + (height * 0.55));
    cairo_line_to(cr, charger_left - (line_width/3*2), top + (height * 0.7));
    cairo_stroke(cr);
    cairo_move_to(cr, charger_left + (line_width/2), top + (height * 0.55));
    cairo_line_to(cr, charger_left + (line_width/2), top + (height * 0.7));
    cairo_stroke(cr);

    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}
static GdkPixbuf* init_outline(int size)
{
    GdkPixbuf* dest = NULL;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);

    const double top = size / 4;
    const double left = size * 0.28;
    const double bottom = size - (top / 2);
    const double right = size - left;
    const double width = right - left;
    const double height = bottom - top;
    const double line_width = height / 10;

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // left side
    cairo_move_to(cr, left, top);
    cairo_line_to(cr, left, bottom);
    cairo_stroke(cr);
    // right side
    cairo_move_to(cr, right, top);
    cairo_line_to(cr, right, bottom);
    cairo_stroke(cr);

    // top side
    cairo_move_to(cr, left, top);
    cairo_line_to(cr, right, top);
    cairo_stroke(cr);

    // bottom side
    cairo_move_to(cr, left, bottom);
    cairo_line_to(cr, right, bottom);
    cairo_stroke(cr);

    // knob thingy
    cairo_set_line_width(cr, line_width * 3);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, left + (width*0.38), top - (line_width * 1.2));
    cairo_line_to(cr, right - (width*0.38), top - (line_width * 1.2));
    cairo_stroke(cr);


    dest = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);

    return dest;
}

GtkWidget* battery_widget(void)
{
    GtkWidget* icon = gtk_drawing_area_new();

    struct PowerStats stats;
    gboolean success = power_devices(&stats);
    if (!success || stats.type == POWER_LINE) {
        return icon;
    }

    struct BatteryWidgetInfo* info = malloc(sizeof(struct BatteryWidgetInfo));
    info->widget = icon;
    info->is_charging = stats.type == POWER_BATTERY_CHARGING;
    info->battery_level = stats.charge;

    info->outline = init_outline(27);
    info->charger = init_charger(27);

    gtk_widget_set_size_request(icon, 27, 27);
    g_signal_connect(G_OBJECT(icon), "draw", G_CALLBACK(draw_battery_widget), info);

    g_timeout_add_seconds(15, G_SOURCE_FUNC(update_battery_status), info);

    return icon;
}

static gboolean update_battery_status(struct BatteryWidgetInfo* info)
{
    struct PowerStats stats;
    gboolean success = power_devices(&stats);
    if (!success) return TRUE;

    info->battery_level = stats.charge;
    info->is_charging = stats.type == POWER_BATTERY_CHARGING;

    gtk_widget_queue_draw(info->widget);
    return TRUE;
}
