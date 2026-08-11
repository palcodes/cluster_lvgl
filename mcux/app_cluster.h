/*
 * app_cluster.h
 *
 * The three calls that connect the cluster to an MCUXpresso SDK project.
 * Everything above them is portable LVGL; everything below is the SDK's
 * display_support / lv_port_disp / lv_port_indev, which you keep as-is.
 */

#ifndef APP_CLUSTER_H
#define APP_CLUSTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build the screen and start the data layer.
 * Call once, after lv_init() + lv_port_disp_init() + lv_port_indev_init().
 */
void app_cluster_init(void);

/**
 * Service LVGL.  Call from your main loop (bare metal) or from the LVGL
 * task (FreeRTOS).  Returns the number of milliseconds until it next
 * wants servicing, which you can feed to vTaskDelay().
 */
uint32_t app_cluster_poll(void);

/**
 * Advance LVGL's clock by 1 ms.  Call from SysTick_Handler(), a PIT ISR,
 * or wherever your project already has a 1 kHz tick.
 *
 * Skip this if your lv_conf.h sets LV_TICK_CUSTOM 1 - LVGL then reads the
 * time itself and calling this as well would run the clock at double rate.
 */
void app_cluster_tick_1ms(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CLUSTER_H */
