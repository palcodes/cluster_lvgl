/*
 * cluster_data.h
 *
 * The vehicle boundary.  Everything the cluster can show, expressed as
 * plain integers and strings, with no LVGL type in the signature.
 *
 * Integer only - there is no floating point in the update path, so this
 * works unchanged on a part without an FPU.  Fractional quantities are
 * passed in their smallest useful unit and formatted here.
 *
 * Call cluster_data_init() once after cl_screen_create(); after that the
 * setters can be called from wherever the CAN frames land.
 */

#ifndef CLUSTER_DATA_H
#define CLUSTER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cl_screen.h"

/* Which way the next manoeuvre goes.  The frame ships the arrow for LEFT;
 * the others reuse the same glyph until the design supplies its own. */
typedef enum {
    CL_TURN_LEFT = 0,
    CL_TURN_RIGHT,
    CL_TURN_STRAIGHT,
    CL_TURN_UTURN,
} cl_manoeuvre_t;

/**
 * Bind the data layer to a screen and push the frame's own values into it,
 * so a freshly built cluster shows the design exactly as drawn.
 */
void cluster_data_init(cl_screen_t *ui);

/* --- the ride ---------------------------------------------------------- */
void cluster_set_speed(int kmh);                  /* 0..999                */
void cluster_set_mode(cl_mode_t mode);
void cluster_set_odometer(uint32_t km);
void cluster_set_range(int km);

/* --- the battery ------------------------------------------------------- */
void cluster_set_soc(int percent);                /* 0..100                */

/* --- navigation -------------------------------------------------------- */
/** Street name and distance in metres; metres roll over to km past 1000.
 *  Pass a NULL street or a negative distance to blank the guidance. */
void cluster_set_nav(const char *street, int metres, cl_manoeuvre_t turn);

/* --- indicators and tell-tales ----------------------------------------- */
void cluster_set_turn_signals(bool left, bool right);
void cluster_set_warning(bool on);
void cluster_set_beam(bool on);
void cluster_set_info(bool on);                   /* the node 1:6 lamp     */

/* --- connectivity ------------------------------------------------------ */
void cluster_set_clock(int hour24, int minute);
void cluster_set_network(const char *label);      /* "4G", "LTE", "--"     */
void cluster_set_bluetooth(bool linked);

/* --- auto balance ------------------------------------------------------ */
/** `on` swaps the hint line for the engaged caption. */
void cluster_set_auto_balance(bool on);

#ifdef __cplusplus
}
#endif

#endif /* CLUSTER_DATA_H */
