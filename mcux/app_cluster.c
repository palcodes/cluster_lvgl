/*
 * app_cluster.c
 *
 * Glue between the SDK example's main() and the cluster UI.
 */

#include "lvgl.h"
#include "app_cluster.h"
#include "cl_screen.h"
#include "cluster_data.h"

/* Longest the service loop may sleep between calls to lv_timer_handler().
 * Matches LV_DEF_REFR_PERIOD - see the note in app_cluster_poll(). */
#define CL_POLL_MAX_MS 30u

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
    uint32_t next = lv_timer_handler();

    /* LVGL 9 returns LV_NO_TIMER_READY (0xFFFFFFFF) when no timer is due,
     * which this screen reaches as soon as it has settled - nothing here
     * animates.  A caller doing vTaskDelay(app_cluster_poll()) would then
     * park for 49 days and the panel would never pick up the next
     * cluster_set_*(), so the idle wait is capped instead. */
    if (next > CL_POLL_MAX_MS) {
        next = CL_POLL_MAX_MS;
    }
    return next;
}

void app_cluster_tick_1ms(void)
{
    lv_tick_inc(1);
}
