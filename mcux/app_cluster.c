/*
 * app_cluster.c
 *
 * Glue between the SDK example's main() and the cluster UI.
 */

#include "lvgl.h"
#include "app_cluster.h"
#include "cl_screen.h"
#include "cluster_data.h"

void app_cluster_init(void)
{
    /* cl_screen_create() builds every widget and loads the screen;
     * cluster_data_init() then writes the frame's own values into it, so the
     * panel comes up showing the design.  Once a CAN or BMS task is calling
     * the cluster_set_*() functions it will overwrite them - there is no
     * demo loop to remove. */
    cl_screen_create(&cl_ui);
    cluster_data_init(&cl_ui);
}

uint32_t app_cluster_poll(void)
{
    return lv_timer_handler();
}

void app_cluster_tick_1ms(void)
{
    lv_tick_inc(1);
}
