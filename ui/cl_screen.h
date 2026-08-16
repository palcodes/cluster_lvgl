/*
 * cl_screen.h
 *
 * The cluster screen: one call builds it, and everything that can change
 * afterwards is reachable through the handle.
 *
 * Nothing in ui/ knows where the numbers come from and nothing in app/
 * knows how they are drawn.  The seam is this header.
 */

#ifndef CL_SCREEN_H
#define CL_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "cl_theme.h"

/* Every widget the vehicle can move.  Anything not listed here is decoration
 * and is created once and never touched again. */
typedef struct {
    lv_obj_t *screen;

    /* status strip */
    lv_obj_t *clock;
    lv_obj_t *soc;
    lv_obj_t *battery;
    lv_obj_t *signal;
    lv_obj_t *nav_led;
    lv_obj_t *bluetooth;

    /* tell-tales */
    lv_obj_t *warning;
    lv_obj_t *warning_halo;
    lv_obj_t *beam;
    lv_obj_t *italic;

    /* navigation */
    lv_obj_t *disc;
    lv_obj_t *manoeuvre;
    lv_obj_t *street;
    lv_obj_t *distance;

    /* speed */
    lv_obj_t *speed;
    lv_obj_t *turn_left;
    lv_obj_t *turn_right;

    /* ride mode selector */
    lv_obj_t *mode_prev;
    lv_obj_t *mode_curr;
    lv_obj_t *mode_next;

    /* right column */
    lv_obj_t *odometer;
    lv_obj_t *range;

    /* auto balance */
    lv_obj_t *ab_title;
    lv_obj_t *ab_hint;
} cl_screen_t;

/* The one instance.  A cluster has exactly one screen; passing the handle
 * around anyway keeps the drawing code free of globals. */
extern cl_screen_t cl_ui;

/**
 * Build the screen and load it.  Call once, after lv_init() and after a
 * display driver is registered.
 */
void cl_screen_create(cl_screen_t *ui);

/**
 * Move the ride-mode selector.  The pill always shows the active mode with
 * the one before it above and the one after below, wrapping at the ends.
 */
void cl_screen_set_mode(cl_screen_t *ui, cl_mode_t mode);

/** Name of a ride mode, as the frame spells it. */
const char *cl_mode_name(cl_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* CL_SCREEN_H */
