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

/** Call in place of lv_timer_handler().  Returns the same thing it does. */
uint32_t app_cluster_poll(void);

/** Call from a 1 ms tick if the port does not already drive lv_tick_inc(). */
void app_cluster_tick_1ms(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CLUSTER_H */
