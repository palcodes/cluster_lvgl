/*
 * app_cluster.h
 *
 * The three calls an MCUXpresso project needs to host this cluster.  Drop
 * these in place of whatever lv_demo_* the SDK example was running; see
 * mcux/README.md for the rest of the bring-up.
 */

#ifndef APP_CLUSTER_H
#define APP_CLUSTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Build the screen and seed it with the design's values.  Call once, after
 * lv_init() and after the display driver is registered.
 */
void app_cluster_init(void);

/**
 * Call in place of lv_timer_handler().  Returns the milliseconds to wait
 * before the next call - lv_timer_handler()'s own answer, except that its
 * idle LV_NO_TIMER_READY is capped to a refresh period so the value can be
 * passed straight to vTaskDelay() or a sleep.
 */
uint32_t app_cluster_poll(void);

/** Call from a 1 ms tick if the port does not already drive lv_tick_inc(). */
void app_cluster_tick_1ms(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CLUSTER_H */
