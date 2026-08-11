/*
 * app_cluster.c
 *
 * Glue between the SDK example's main() and the cluster UI.
 */

#include "lvgl.h"
#include "app_cluster.h"
#include "ui_dash.h"
#include "ev_data.h"

void app_cluster_init(void)
{
    /* Builds every widget, loads the screen, and starts the power-on
     * reveal.  ev_data_init() then seeds the values and kicks off the demo
     * ride - delete the ev_data_demo_start() call inside it once your CAN
     * or BMS task is feeding the ev_data_set_*() functions instead. */
    ev_dash_create(&ev_ui);
    ev_data_init(&ev_ui);
}

uint32_t app_cluster_poll(void)
{
    return lv_timer_handler();
}

void app_cluster_tick_1ms(void)
{
    lv_tick_inc(1);
}
