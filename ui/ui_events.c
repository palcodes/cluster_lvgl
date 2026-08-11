/*
 * ui_events.c
 *
 * The only interaction on this screen: picking a ride mode.
 */

#include "ui_dash.h"
#include "ev_data.h"

static void chip_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ev_data_set_mode((ev_mode_t)(lv_uintptr_t)lv_event_get_user_data(e));
}

void ev_events_attach(ev_ui_t *ui)
{
    int i;
    for (i = 0; i < 3; i++) {
        lv_obj_add_event_cb(ui->chip[i], chip_clicked, LV_EVENT_CLICKED,
                            (void *)(lv_uintptr_t)i);
    }
}
