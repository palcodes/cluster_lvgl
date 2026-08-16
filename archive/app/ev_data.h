/*
 * ev_data.h
 *
 * The boundary between the vehicle and the cluster.  Nothing in ui/ knows
 * where the numbers come from; nothing here knows how they are drawn.
 *
 * All integer - no floating point anywhere in the render path.
 */

#ifndef EV_DATA_H
#define EV_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_dash.h"

/** Bind the data layer to a built screen, then start the demo ride. */
void ev_data_init(ev_ui_t *ui);

/* ---- live values ----------------------------------------------------- */

/** Road speed in km/h; eases the ring and the numeral into place. */
void ev_data_set_speed(int32_t kmh);

/** State of charge in %, remaining range in km. */
void ev_data_set_battery(int32_t soc_pct, int32_t range_km);

/** Motor power in watts.  Negative means regenerative braking. */
void ev_data_set_power(int32_t watts);

/** Temperatures in degrees Celsius. */
void ev_data_set_temps(int32_t motor_c, int32_t cell_c, int32_t ambient_c);

/** Trip A in hundredths of a km (2360 = 23.60 km) and odometer in km. */
void ev_data_set_distance(uint32_t trip_hm, uint32_t odo_km);

/** Turn-by-turn: distance to the manoeuvre in metres, the street it puts
 *  you on, and which way to go.  Pass 0 metres to clear the guidance. */
void ev_data_set_nav(uint32_t metres, const char *street, ev_nav_t manoeuvre);

void ev_data_set_mode(ev_mode_t mode);
void ev_data_set_turn(bool left, bool right);
void ev_data_set_charging(bool charging);
void ev_data_set_beam(bool on);
void ev_data_set_clock(int32_t hour, int32_t minute);
void ev_data_set_ride_time(uint32_t seconds);

/* ---- demo ------------------------------------------------------------ */

void ev_data_demo_start(void);
void ev_data_demo_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* EV_DATA_H */
