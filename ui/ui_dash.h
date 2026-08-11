/*
 * ui_dash.h
 *
 * The cluster screen: widget handles and the functions that drive it.
 */

#ifndef UI_DASH_H
#define UI_DASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_theme.h"

typedef struct
{
    lv_obj_t *scr;

    /* ---- top strip --------------------------------------------------- */
    lv_obj_t *batt_shell;       /* drawn battery outline                  */
    lv_obj_t *batt_cap;         /* the little nub on the right            */
    lv_obj_t *batt_fill;        /* charge level inside the shell          */
    lv_obj_t *lb_soc;
    lv_obj_t *lb_bolt;          /* charging, hidden when unplugged        */
    lv_obj_t *lb_mode_name;
    lv_obj_t *lb_turn_l;
    lv_obj_t *lb_turn_r;
    lv_obj_t *lb_beam;
    lv_obj_t *lb_clock;
    lv_obj_t *lb_link;          /* bluetooth / connectivity               */

    /* ---- centre: halo + speed ---------------------------------------- */
    lv_obj_t *glow_out;         /* soft bloom outside the ring            */
    lv_obj_t *glow_in;          /* breathing bloom inside                 */
    lv_obj_t *halo[3];          /* slow counter-rotating rings            */
    lv_obj_t *halo_crest;       /* the bright short arc                   */
    lv_obj_t *arc_ring;         /* speed track; also stores current speed */
    lv_obj_t *arc_seg[EV_ARC_SEGMENTS];  /* gradient fill                 */
    lv_obj_t *lb_speed;
    lv_obj_t *lb_speed_unit;
    lv_obj_t *lb_scale_min;
    lv_obj_t *lb_scale_max;

    /* ---- left column: navigation, trip, motor ------------------------ */
    lv_obj_t *cap_nav;
    lv_obj_t *lb_nav_icon;
    lv_obj_t *lb_nav_dist;
    lv_obj_t *lb_nav_street;
    lv_obj_t *rule_1;
    lv_obj_t *cap_trip;
    lv_obj_t *lb_trip;
    lv_obj_t *rule_2;
    lv_obj_t *cap_motor;
    lv_obj_t *lb_motor;

    /* ---- right column: range, power, cell ---------------------------- */
    lv_obj_t *cap_range;
    lv_obj_t *lb_range;
    lv_obj_t *rule_3;
    lv_obj_t *cap_power;
    lv_obj_t *lb_power;
    lv_obj_t *bar_power;
    lv_obj_t *power_zero;
    lv_obj_t *lb_regen;
    lv_obj_t *lb_drive;
    lv_obj_t *rule_4;
    lv_obj_t *cap_cell;
    lv_obj_t *lb_cell;

    /* ---- footer ------------------------------------------------------ */
    lv_obj_t *rule_foot;
    lv_obj_t *cap_odo;
    lv_obj_t *lb_odo;
    lv_obj_t *chip[3];          /* STREET / CRAWL / RUSH                  */
    lv_obj_t *chip_lb[3];
    lv_obj_t *cap_ride;
    lv_obj_t *lb_ride;

} ev_ui_t;

extern ev_ui_t ev_ui;

/** Build the screen and load it. */
void ev_dash_create(ev_ui_t *ui);

/** Animate the ring + numeric readout to `target` km/h. */
void ev_dash_anim_speed(ev_ui_t *ui, int32_t target, uint32_t time_ms);

/** Repaint the ring gradient, halo and tell-tales for a ride mode. */
void ev_dash_apply_mode(ev_ui_t *ui, ev_mode_t mode);

/** Power-on reveal: staggered fade, halo spin-up, full-scale sweep. */
void ev_dash_boot(ev_ui_t *ui);

/** Start the endless halo drift.  Called by ev_dash_boot() once the
 *  spin-up has decelerated; exposed so you can restart it after a sleep. */
void ev_dash_halo_drift(ev_ui_t *ui);

/** Attach the ride-mode chip handlers. */
void ev_events_attach(ev_ui_t *ui);

#ifdef __cplusplus
}
#endif

#endif /* UI_DASH_H */
