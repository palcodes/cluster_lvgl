/*
 * ev_data.c
 *
 * Value setters plus a self-contained ride simulation, so the cluster is
 * alive the moment it boots.  Replace ev_data_demo_start() with your CAN /
 * BMS / navigation plumbing and call the setters instead.
 */

#include "ev_data.h"

#define EV_DEMO_PERIOD_MS   100
#define EV_BOOT_HOLD_MS     2600    /* let the power-on sequence finish */

#define EV_BATT_FILL_MAX    22      /* px inside the battery outline    */

static ev_ui_t    *ui = NULL;
static lv_timer_t *demo_timer = NULL;

/* Held so the combined "28C · 18:42" strip can be rebuilt from either half. */
static int32_t s_ambient = 28;
static int32_t s_hour = 18, s_minute = 42;

static ev_mode_t s_mode = EV_MODE_STREET;
static int       s_batt_state = -1;     /* 0 ok, 1 low, 2 critical */

/**********************
 *      UTILITIES
 **********************/
static int32_t ev_abs(int32_t v)   { return v < 0 ? -v : v; }

static int32_t ev_clamp(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void refresh_status_strip(void)
{
    if (ui == NULL) return;
    lv_label_set_text_fmt(ui->lb_clock, "%d" EV_DEG "C" EV_MIDDOT "%02d:%02d",
                          (int)s_ambient, (int)s_hour, (int)s_minute);
}

/* Turn-indicator blink. */
static void blink_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

static void blink_set(lv_obj_t *label, bool on)
{
    lv_anim_t a;

    if (label == NULL) return;
    lv_anim_del(label, blink_opa_cb);

    if (!on) {
        lv_obj_set_style_opa(label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_exec_cb(&a, blink_opa_cb);
    lv_anim_set_values(&a, LV_OPA_COVER, 25);
    lv_anim_set_time(&a, 320);
    lv_anim_set_playback_time(&a, 320);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/**********************
 *    VALUE SETTERS
 **********************/

void ev_data_set_speed(int32_t kmh)
{
    if (ui == NULL) return;
    ev_dash_anim_speed(ui, kmh, 220);
}

void ev_data_set_battery(int32_t soc_pct, int32_t range_km)
{
    lv_coord_t w;
    int state;

    if (ui == NULL) return;

    soc_pct = ev_clamp(soc_pct, 0, 100);

    lv_label_set_text_fmt(ui->lb_soc, "%d%%", (int)soc_pct);
    lv_label_set_text_fmt(ui->lb_range, "%d", (int)range_km);

    w = (lv_coord_t)((EV_BATT_FILL_MAX * soc_pct) / 100);
    if (w < 2 && soc_pct > 0) w = 2;
    lv_obj_set_width(ui->batt_fill, w);

    /* Below 20 % the battery stops following the mode accent and warns.
     * Only repaint on a state change, otherwise restoring the mode colours
     * would fight the accent every tick. */
    state = (soc_pct < 10) ? 2 : (soc_pct < 20) ? 1 : 0;
    if (state == s_batt_state) return;
    s_batt_state = state;

    if (state == 0) {
        ev_dash_apply_mode(ui, s_mode);         /* restores the gradient */
        lv_obj_set_style_border_color(ui->batt_shell, EV_INK_MUTE, LV_PART_MAIN);
        lv_obj_set_style_text_color(ui->lb_soc, EV_INK, LV_PART_MAIN);
    } else {
        lv_color_t warn = (state == 2) ? EV_RED : EV_AMBER;
        lv_obj_set_style_bg_color(ui->batt_fill, warn, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(ui->batt_fill, warn, LV_PART_MAIN);
        lv_obj_set_style_border_color(ui->batt_shell, warn, LV_PART_MAIN);
        lv_obj_set_style_text_color(ui->lb_soc, warn, LV_PART_MAIN);
    }
}

void ev_data_set_power(int32_t watts)
{
    int32_t shown;

    if (ui == NULL) return;

    watts = ev_clamp(watts, -2000, 6000);
    lv_bar_set_value(ui->bar_power, watts, LV_ANIM_ON);

    /* kW to one decimal without dragging in floating-point printf. */
    shown = ev_abs(watts);
    lv_label_set_text_fmt(ui->lb_power, "%s%d.%d",
                          watts < 0 ? "-" : "",
                          (int)(shown / 1000), (int)((shown % 1000) / 100));

    if (watts < 0) {
        lv_obj_set_style_bg_color(ui->bar_power, EV_GREEN, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(ui->bar_power, EV_TEAL, LV_PART_INDICATOR);
        lv_obj_set_style_shadow_color(ui->bar_power, EV_GREEN, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(ui->lb_regen, EV_GREEN, LV_PART_MAIN);
        lv_obj_set_style_text_color(ui->lb_drive, EV_INK_MUTE, LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(ui->lb_regen, EV_INK_MUTE, LV_PART_MAIN);
        lv_obj_set_style_text_color(ui->lb_drive, EV_INK_DIM, LV_PART_MAIN);
    }
}

void ev_data_set_temps(int32_t motor_c, int32_t cell_c, int32_t ambient_c)
{
    if (ui == NULL) return;

    lv_label_set_text_fmt(ui->lb_motor, "%d", (int)motor_c);
    lv_label_set_text_fmt(ui->lb_cell,  "%d", (int)cell_c);
    lv_obj_set_style_text_color(ui->lb_motor,
                                motor_c >= 90 ? EV_AMBER : EV_INK, LV_PART_MAIN);

    s_ambient = ambient_c;
    refresh_status_strip();
}

void ev_data_set_distance(uint32_t trip_hm, uint32_t odo_km)
{
    if (ui == NULL) return;

    lv_label_set_text_fmt(ui->lb_trip, "%d.%d",
                          (int)(trip_hm / 100), (int)((trip_hm % 100) / 10));

    if (odo_km >= 1000) {
        lv_label_set_text_fmt(ui->lb_odo, "%d,%03d km",
                              (int)(odo_km / 1000), (int)(odo_km % 1000));
    } else {
        lv_label_set_text_fmt(ui->lb_odo, "%d km", (int)odo_km);
    }
}

void ev_data_set_nav(uint32_t metres, const char *street, ev_nav_t manoeuvre)
{
    const char *glyph;

    if (ui == NULL) return;

    if (metres == 0) {
        lv_obj_add_flag(ui->lb_nav_icon, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui->lb_nav_dist, "");
        lv_label_set_text(ui->lb_nav_street, "No route");
        return;
    }
    lv_obj_clear_flag(ui->lb_nav_icon, LV_OBJ_FLAG_HIDDEN);

    switch (manoeuvre) {
        case EV_NAV_LEFT:  glyph = EV_ICON_TURN_L;  break;
        case EV_NAV_RIGHT: glyph = EV_ICON_TURN_R;  break;
        case EV_NAV_UTURN: glyph = EV_ICON_UTURN;   break;
        case EV_NAV_STRAIGHT:
        default:           glyph = EV_ICON_ARROW_U; break;
    }
    lv_label_set_text(ui->lb_nav_icon, glyph);

    /* Under a kilometre count in metres, above it switch to km with one
     * decimal - the same convention every nav app uses. */
    if (metres >= 1000) {
        lv_label_set_text_fmt(ui->lb_nav_dist, "%d.%d km",
                              (int)(metres / 1000), (int)((metres % 1000) / 100));
    } else {
        lv_label_set_text_fmt(ui->lb_nav_dist, "%d m", (int)(metres / 10 * 10));
    }

    lv_label_set_text(ui->lb_nav_street, street);

    /* Inside 120 m the manoeuvre glyph lights up in the mode accent. */
    lv_obj_set_style_text_color(ui->lb_nav_icon,
                                metres <= 120 ? EV_CYAN : EV_INK, LV_PART_MAIN);
}

void ev_data_set_mode(ev_mode_t mode)
{
    s_mode = mode;
    if (ui == NULL) return;
    ev_dash_apply_mode(ui, mode);
    s_batt_state = -1;      /* force the battery to re-evaluate its colours */
}

void ev_data_set_turn(bool left, bool right)
{
    if (ui == NULL) return;
    blink_set(ui->lb_turn_l, left);
    blink_set(ui->lb_turn_r, right);
}

void ev_data_set_charging(bool charging)
{
    if (ui == NULL) return;
    if (charging) lv_obj_clear_flag(ui->lb_bolt, LV_OBJ_FLAG_HIDDEN);
    else          lv_obj_add_flag(ui->lb_bolt, LV_OBJ_FLAG_HIDDEN);
}

void ev_data_set_beam(bool on)
{
    if (ui == NULL) return;
    lv_obj_set_style_text_color(ui->lb_beam, on ? EV_CYAN : EV_INK_MUTE,
                                LV_PART_MAIN);
}

void ev_data_set_clock(int32_t hour, int32_t minute)
{
    s_hour = hour;
    s_minute = minute;
    refresh_status_strip();
}

void ev_data_set_ride_time(uint32_t seconds)
{
    if (ui == NULL) return;
    lv_label_set_text_fmt(ui->lb_ride, "%02d:%02d",
                          (int)(seconds / 60), (int)(seconds % 60));
}

/**********************
 *   DEMO SIMULATION
 **********************/
/* A city ride: speed follows two summed sines, power falls out of speed and
 * acceleration, the pack drains, and the route steps through a short list of
 * manoeuvres.  Cosmetic only - delete once real data arrives. */

typedef struct {
    uint32_t metres;
    const char *street;
    ev_nav_t   manoeuvre;
} nav_step_t;

static const nav_step_t nav_route[] = {
    {  960, "To Linking Road",       EV_NAV_RIGHT    },
    { 1400, "Onto Carter Road",      EV_NAV_LEFT     },
    {  420, "Continue on Carter Rd", EV_NAV_STRAIGHT },
    { 2100, "To Worli Sea Face",     EV_NAV_RIGHT    },
};
#define NAV_STEPS (sizeof(nav_route) / sizeof(nav_route[0]))

static struct {
    uint32_t tick;
    int32_t  speed;
    int32_t  prev_speed;
    int32_t  soc_x10;
    uint32_t trip_hm;
    uint32_t ride_s;
    uint32_t nav_left;
    uint32_t nav_idx;
    bool     turn_l;
    bool     turn_r;
} d;

static void demo_cb(lv_timer_t *timer)
{
    int32_t angle, s, power;
    uint32_t phase_s;

    LV_UNUSED(timer);
    d.tick++;

    if (d.tick * EV_DEMO_PERIOD_MS < EV_BOOT_HOLD_MS) return;

    /* --- speed -------------------------------------------------------- */
    angle = (int32_t)((d.tick * 2) % 360);
    s = 46
      + (34 * lv_trigo_sin((int16_t)angle)) / 32767
      + (11 * lv_trigo_sin((int16_t)((angle * 3) % 360))) / 32767;
    s = ev_clamp(s, 0, EV_SPEED_MAX);

    d.prev_speed = d.speed;
    d.speed = s;
    ev_data_set_speed(s);

    /* --- power -------------------------------------------------------- */
    power = s * 42 + (s - d.prev_speed) * 2600;
    ev_data_set_power(ev_clamp(power, -2000, 6000));

    /* --- battery ------------------------------------------------------ */
    if (d.soc_x10 > 0 && (d.tick % 14) == 0) d.soc_x10--;
    ev_data_set_battery(d.soc_x10 / 10, (d.soc_x10 * 42) / 100);

    /* --- temperatures -------------------------------------------------- */
    ev_data_set_temps(34 + s / 3, 27 + s / 12, 28);

    /* --- distance ------------------------------------------------------ */
    d.trip_hm += (uint32_t)(s / 8) + 1;
    ev_data_set_distance(d.trip_hm, 4812 + d.trip_hm / 100);

    if ((d.tick % 10) == 0) {
        d.ride_s++;
        ev_data_set_ride_time(d.ride_s);
    }

    /* --- navigation: burn down the distance, then take the next step --- */
    {
        uint32_t closed = (uint32_t)(s / 3) + 2;
        if (d.nav_left > closed) {
            d.nav_left -= closed;
        } else {
            d.nav_idx = (d.nav_idx + 1) % NAV_STEPS;
            d.nav_left = nav_route[d.nav_idx].metres;
        }
        ev_data_set_nav(d.nav_left,
                        nav_route[d.nav_idx].street,
                        nav_route[d.nav_idx].manoeuvre);
    }

    /* --- indicators, on a 30 s loop ------------------------------------ */
    phase_s = (d.tick / 10) % 30;
    {
        bool want_l = (phase_s >= 8  && phase_s < 13);
        bool want_r = (phase_s >= 20 && phase_s < 25);
        if (want_l != d.turn_l || want_r != d.turn_r) {
            d.turn_l = want_l;
            d.turn_r = want_r;
            ev_data_set_turn(want_l, want_r);
        }
    }
}

void ev_data_demo_start(void)
{
    if (demo_timer != NULL) return;

    lv_memset_00(&d, sizeof(d));
    d.soc_x10   = 860;                      /* 86.0 %        */
    d.trip_hm   = 2360;                     /* 23.60 km      */
    d.nav_idx   = 0;
    d.nav_left  = nav_route[0].metres;

    demo_timer = lv_timer_create(demo_cb, EV_DEMO_PERIOD_MS, NULL);
}

void ev_data_demo_stop(void)
{
    if (demo_timer == NULL) return;
    lv_timer_del(demo_timer);
    demo_timer = NULL;
}

/**********************
 *        INIT
 **********************/
void ev_data_init(ev_ui_t *screen)
{
    ui = screen;

    ev_data_set_clock(18, 42);
    ev_data_set_beam(true);
    ev_data_set_mode(EV_MODE_STREET);
    ev_data_demo_start();
}
