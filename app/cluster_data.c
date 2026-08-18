/*
 * cluster_data.c
 *
 * The setters behind cluster_data.h.  Each one formats a value and writes
 * it to a widget - no layout decisions, no invented state colours.  Where a
 * value can be absent (guidance, indicators) the widget is hidden rather
 * than blanked, so nothing shifts around it.
 *
 * cluster_data_init() seeds every field with the value the Figma frame
 * shows, which is what makes a freshly built screen comparable against the
 * design pixel for pixel.
 */

#include "cluster_data.h"

#include <stdio.h>
#include <string.h>

static cl_screen_t *s_ui;

/* Hide rather than blank: a hidden object keeps its slot, so nothing
 * reflows when a tell-tale goes out. */
static void show(lv_obj_t *o, bool on)
{
    if (!o) return;
    if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

/*--------------------------------------------------------------------------
 *  The ride
 *-------------------------------------------------------------------------*/

void cluster_set_speed(int kmh)
{
    char buf[8];
    if (kmh < 0)   kmh = 0;
    if (kmh > 999) kmh = 999;
    snprintf(buf, sizeof buf, "%d", kmh);
    lv_label_set_text(s_ui->speed, buf);
}

void cluster_set_mode(cl_mode_t mode)
{
    cl_screen_set_mode(s_ui, mode);
}

void cluster_set_odometer(uint32_t km)
{
    char buf[12];
    snprintf(buf, sizeof buf, "%lu", (unsigned long)km);
    lv_label_set_text(s_ui->odometer, buf);
}

void cluster_set_range(int km)
{
    char buf[16];
    if (km < 0) km = 0;
    snprintf(buf, sizeof buf, "%d KM", km);
    lv_label_set_text(s_ui->range, buf);
}

/*--------------------------------------------------------------------------
 *  The battery
 *-------------------------------------------------------------------------*/

void cluster_set_soc(int percent)
{
    char buf[8];
    if (percent < 0)   percent = 0;
    if (percent > 100) percent = 100;
    snprintf(buf, sizeof buf, "%d%%", percent);
    lv_label_set_text(s_ui->soc, buf);
}

/*--------------------------------------------------------------------------
 *  Navigation
 *-------------------------------------------------------------------------*/

void cluster_set_nav(const char *street, int metres, cl_manoeuvre_t turn)
{
    char buf[24];
    bool on = (street != NULL) && (metres >= 0);

    LV_UNUSED(turn);            /* one arrow in the frame so far */

    show(s_ui->disc, on);
    show(s_ui->manoeuvre, on);
    show(s_ui->street, on);
    show(s_ui->distance, on);
    show(s_ui->nav_led, on);
    if (!on) return;

    lv_label_set_text(s_ui->street, street);

    /* Past a kilometre every nav app switches units, and so does this. */
    if (metres >= 1000) {
        snprintf(buf, sizeof buf, "%d.%d km away", metres / 1000,
                 (metres % 1000) / 100);
    } else {
        snprintf(buf, sizeof buf, "%dm away", metres);
    }
    lv_label_set_text(s_ui->distance, buf);
}

/*--------------------------------------------------------------------------
 *  Indicators and tell-tales
 *-------------------------------------------------------------------------*/

void cluster_set_turn_signals(bool left, bool right)
{
    show(s_ui->turn_left, left);
    show(s_ui->turn_right, right);
}

void cluster_set_warning(bool on)
{
    show(s_ui->warning, on);
    show(s_ui->warning_halo, on);
}

void cluster_set_beam(bool on)
{
    show(s_ui->beam, on);
}

void cluster_set_info(bool on)
{
    show(s_ui->italic, on);
}

/*--------------------------------------------------------------------------
 *  Connectivity
 *-------------------------------------------------------------------------*/

void cluster_set_clock(int hour24, int minute)
{
    char buf[12];
    const char *suffix = (hour24 >= 12) ? "PM" : "AM";
    int h12 = hour24 % 12;
    if (h12 == 0) h12 = 12;
    snprintf(buf, sizeof buf, "%02d:%02d %s", h12, minute, suffix);
    lv_label_set_text(s_ui->clock, buf);
}

void cluster_set_network(const char *label)
{
    lv_label_set_text(s_ui->signal, label ? label : "");
}

void cluster_set_bluetooth(bool linked)
{
    show(s_ui->bluetooth, linked);
}

/*--------------------------------------------------------------------------
 *  Auto balance
 *-------------------------------------------------------------------------*/

void cluster_set_auto_balance(bool on)
{
    lv_label_set_text(s_ui->ab_hint, on ? "Engaged" : "Press AB to turn on");
}

/*--------------------------------------------------------------------------
 *  Seeding
 *-------------------------------------------------------------------------*/

void cluster_data_init(cl_screen_t *ui)
{
    s_ui = ui;

    cluster_set_speed(56);
    cluster_set_mode(CL_MODE_STREET);
    cluster_set_odometer(8999999u);
    cluster_set_range(72);

    cluster_set_soc(89);

    cluster_set_nav("CARTER ROAD", 200, CL_TURN_LEFT);

    cluster_set_turn_signals(true, true);
    cluster_set_warning(true);
    cluster_set_beam(true);
    cluster_set_info(true);

    cluster_set_clock(17, 35);
    cluster_set_network("4G");
    cluster_set_bluetooth(true);

    cluster_set_auto_balance(false);
}
