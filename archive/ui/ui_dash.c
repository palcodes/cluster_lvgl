/*
 * ui_dash.c
 *
 * EV two-wheeler cluster, 800 x 480.
 *
 *   +-----------------------------------------------------------------+
 *   | [==] 85%  STREET   (o)        < >         (*) 28C · 18:42    ᛒ  |
 *   |                                                                 |
 *   |  NAVIGATION            .-'''''''''-.            RANGE · KM      |
 *   |  ^  960 m           .'  soft  halo  '.                 356      |
 *   |  To Santa Monica  .'      73          '.                        |
 *   |  ---------------  '       KMH         '  ---------------        |
 *   |  TRIP A · KM       '.               .'   POWER · kW             |
 *   |  23.6                '-.._____..-'              2.4             |
 *   |                                          [--|=====]             |
 *   |  ---------------                         ---------------        |
 *   |  MOTOR · C                               CELL · C               |
 *   |  42                                      31                     |
 *   |  --------------------------------------------------------       |
 *   |  ODOMETER      [ STREET ][ CRAWL ][ RUSH ]      RIDE TIME       |
 *   |  4,812 km                                           00:42       |
 *   +-----------------------------------------------------------------+
 *
 * Everything floats on pure black - no panels, no borders except hairlines.
 * The centre is a speed ring built from EV_ARC_SEGMENTS abutting arcs (LVGL
 * arcs are single-colour, so that is how the gradient is made), wrapped
 * around three counter-rotating halo rings and a breathing bloom.
 */

#include "ui_dash.h"
#include <stdbool.h>

ev_ui_t ev_ui;

/**********************
 *   LOCAL PROTOTYPES
 **********************/
static void anim_opa_cb(void *var, int32_t v);
static void anim_arc_opa_cb(void *var, int32_t v);
static void anim_y_cb(void *var, int32_t v);
static void anim_rot_cb(void *var, int32_t v);
static void anim_shadow_cb(void *var, int32_t v);
static void anim_speed_cb(void *var, int32_t v);
static void drift_handover_cb(lv_timer_t *t);

static void ev_fade_in(lv_obj_t *obj, uint32_t time, uint32_t delay);
static void ev_rise_in(lv_obj_t *obj, lv_coord_t y_end, lv_coord_t dy,
                       uint32_t time, uint32_t delay);
static void ev_spin(lv_obj_t *obj, bool cw, uint32_t period, uint32_t delay);

static lv_obj_t *ev_label(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                          const char *txt, const lv_font_t *font,
                          lv_color_t col);
static lv_obj_t *ev_label_r(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                            lv_coord_t w, const char *txt,
                            const lv_font_t *font, lv_color_t col);
static lv_obj_t *ev_caption(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                            const char *txt);
static lv_obj_t *ev_caption_r(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, const char *txt);
static lv_obj_t *ev_rule(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                         lv_coord_t w, bool fade_right);
static lv_obj_t *ev_block(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                          lv_coord_t w, lv_coord_t h);
static lv_obj_t *ev_glow(lv_obj_t *par, lv_coord_t size, lv_coord_t blur,
                         lv_coord_t spread, lv_opa_t opa);
static lv_obj_t *ev_halo_ring(lv_obj_t *par, lv_coord_t size, lv_coord_t w,
                              uint16_t span, lv_opa_t opa);
static void ev_chip_paint(lv_obj_t *chip, lv_obj_t *label, bool on,
                          lv_color_t accent);

/**********************
 *  STATIC VARIABLES
 **********************/
static ev_ui_t *s_ui = NULL;

/**********************
 *   SCREEN BUILDER
 **********************/
void ev_dash_create(ev_ui_t *ui)
{
    int i;

    s_ui = ui;

    /*=================================================================
     * GROUND
     *================================================================*/
    ui->scr = lv_obj_create(NULL);
    lv_obj_set_size(ui->scr, EV_W, EV_H);
    lv_obj_clear_flag(ui->scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui->scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(ui->scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->scr, lv_color_hex(0x04060B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(ui->scr, EV_BLACK, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(ui->scr, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui->scr, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui->scr, EV_FONT_UI, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->scr, EV_INK, LV_PART_MAIN);

    /*=================================================================
     * TOP STRIP
     *================================================================*/

    /* Battery drawn from three primitives rather than a glyph, so the fill
     * can animate and recolour with charge state. */
    ui->batt_shell = ev_block(ui->scr, 24, 15, 30, 16);
    lv_obj_set_style_radius(ui->batt_shell, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->batt_shell, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(ui->batt_shell, EV_INK_MUTE, LV_PART_MAIN);

    ui->batt_cap = ev_block(ui->scr, 55, 20, 3, 6);
    lv_obj_set_style_radius(ui->batt_cap, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->batt_cap, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->batt_cap, EV_INK_MUTE, LV_PART_MAIN);

    ui->batt_fill = ev_block(ui->scr, 26, 17, 22, 12);
    lv_obj_set_style_radius(ui->batt_fill, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->batt_fill, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->batt_fill, EV_BLUE, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(ui->batt_fill, EV_ICE, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(ui->batt_fill, LV_GRAD_DIR_HOR, LV_PART_MAIN);

    ui->lb_soc  = ev_label(ui->scr, 66, 14, "85%", EV_FONT_CAP, EV_INK);
    ui->lb_bolt = ev_label(ui->scr, 100, 14, EV_ICON_BOLT, EV_FONT_CAP, EV_AMBER);
    lv_obj_add_flag(ui->lb_bolt, LV_OBJ_FLAG_HIDDEN);

    ui->lb_mode_name = ev_label(ui->scr, 126, 14, "STREET", EV_FONT_CAP, EV_BLUE);
    lv_obj_set_style_text_letter_space(ui->lb_mode_name, 3, LV_PART_MAIN);

    ui->lb_beam = ev_label(ui->scr, 200, 13, EV_ICON_EYE, EV_FONT_CAP, EV_INK_MUTE);

    /* Turn indicators, symmetric about the screen centre line */
    ui->lb_turn_l = ev_label(ui->scr, 340, 10, EV_ICON_ARROW_L, EV_FONT_ICON, EV_GREEN);
    lv_obj_add_flag(ui->lb_turn_l, LV_OBJ_FLAG_HIDDEN);
    ui->lb_turn_r = ev_label(ui->scr, 434, 10, EV_ICON_ARROW_R, EV_FONT_ICON, EV_GREEN);
    lv_obj_add_flag(ui->lb_turn_r, LV_OBJ_FLAG_HIDDEN);

    ui->lb_clock = ev_label_r(ui->scr, 620, 13, 132,
                              "28" EV_DEG "C" EV_MIDDOT "18:42",
                              EV_FONT_UI, EV_INK_DIM);
    ui->lb_link = ev_label(ui->scr, 762, 14, EV_ICON_BT, EV_FONT_UI, EV_BLUE);

    /*=================================================================
     * CENTRE - bloom, halo, speed ring
     *================================================================*/
    ui->glow_out = ev_glow(ui->scr, EV_GLOW_OUT, 96, -30, 75);
    ui->glow_in  = ev_glow(ui->scr, EV_GLOW_IN,  62, -14, 120);

    ui->halo[0]   = ev_halo_ring(ui->scr, EV_HALO_1, 2, 210, 60);
    ui->halo[1]   = ev_halo_ring(ui->scr, EV_HALO_2, 3, 130, 105);
    ui->halo[2]   = ev_halo_ring(ui->scr, EV_HALO_3, 1, 290, 45);
    /* Deliberately dimmer than the speed ring - it is atmosphere, and must
     * never be mistaken for the value. */
    ui->halo_crest = ev_halo_ring(ui->scr, EV_HALO_CREST, 3, 60, 150);

    /* Speed track.  Its own indicator is hidden; the widget doubles as the
     * store for the current speed so lv_arc_get_value() stays meaningful. */
    ui->arc_ring = lv_arc_create(ui->scr);
    lv_obj_set_pos(ui->arc_ring, EV_CENTER_X(EV_ARC_SIZE), EV_CENTER_Y(EV_ARC_SIZE));
    lv_obj_set_size(ui->arc_ring, EV_ARC_SIZE, EV_ARC_SIZE);
    lv_obj_remove_style(ui->arc_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ui->arc_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(ui->arc_ring, 0, LV_PART_MAIN);
    lv_arc_set_rotation(ui->arc_ring, 0);
    lv_arc_set_bg_angles(ui->arc_ring, 135, 45);
    lv_arc_set_range(ui->arc_ring, 0, EV_SPEED_MAX);
    lv_arc_set_value(ui->arc_ring, 0);
    lv_obj_set_style_arc_width(ui->arc_ring, EV_ARC_W, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui->arc_ring, EV_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(ui->arc_ring, true, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ui->arc_ring, LV_OPA_TRANSP, LV_PART_INDICATOR);

    /* Gradient fill: segment i owns the slice between i/N and (i+1)/N of
     * full scale.  Abutting rounded caps hide the joins. */
    for (i = 0; i < EV_ARC_SEGMENTS; i++) {
        lv_obj_t *seg = lv_arc_create(ui->scr);
        uint16_t a0 = (uint16_t)((135 + (270 * i)       / EV_ARC_SEGMENTS) % 360);
        uint16_t a1 = (uint16_t)((135 + (270 * (i + 1)) / EV_ARC_SEGMENTS) % 360);

        lv_obj_set_pos(seg, EV_CENTER_X(EV_ARC_SIZE), EV_CENTER_Y(EV_ARC_SIZE));
        lv_obj_set_size(seg, EV_ARC_SIZE, EV_ARC_SIZE);
        lv_obj_remove_style(seg, NULL, LV_PART_KNOB);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_pad_all(seg, 0, LV_PART_MAIN);
        lv_arc_set_rotation(seg, 0);
        lv_arc_set_bg_angles(seg, a0, a1);
        lv_arc_set_range(seg, 0, EV_SPEED_MAX / EV_ARC_SEGMENTS);
        lv_arc_set_value(seg, 0);
        lv_obj_set_style_arc_opa(seg, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_width(seg, EV_ARC_W, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(seg, true, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(seg, LV_OPA_COVER, LV_PART_INDICATOR);
        ui->arc_seg[i] = seg;
    }

    ui->lb_speed = ev_label_r(ui->scr, EV_DIAL_CX - 160, 192, 320, "0",
                              EV_FONT_SPEED, EV_INK);
    lv_obj_set_style_text_align(ui->lb_speed, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    ui->lb_speed_unit = ev_label_r(ui->scr, EV_DIAL_CX - 160, 274, 320, "KMH",
                                   EV_FONT_CAP_S, EV_INK_MUTE);
    lv_obj_set_style_text_align(ui->lb_speed_unit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(ui->lb_speed_unit, 6, LV_PART_MAIN);

    ui->lb_scale_min = ev_label(ui->scr, 272, 350, "0", EV_FONT_CAP_S, EV_INK_MUTE);
    ui->lb_scale_max = ev_label_r(ui->scr, 490, 350, 38, "120",
                                  EV_FONT_CAP_S, EV_INK_MUTE);

    /*=================================================================
     * LEFT COLUMN - navigation, trip, motor
     *================================================================*/
    ui->cap_nav = ev_caption(ui->scr, EV_COL_L_X, 90, "NAVIGATION");

    ui->lb_nav_icon = ev_label(ui->scr, EV_COL_L_X, 112, EV_ICON_TURN_R,
                               EV_FONT_ICON_L, EV_INK);
    ui->lb_nav_dist = ev_label(ui->scr, EV_COL_L_X + 52, 108, "960 m",
                               EV_FONT_NUM_L, EV_INK);

    ui->lb_nav_street = ev_label(ui->scr, EV_COL_L_X, 166, "To Santa Monica St.",
                                 EV_FONT_UI, EV_INK_DIM);
    lv_obj_set_width(ui->lb_nav_street, EV_COL_W);
    lv_label_set_long_mode(ui->lb_nav_street, LV_LABEL_LONG_DOT);

    ui->rule_1 = ev_rule(ui->scr, EV_COL_L_X, EV_RULE_1_Y, EV_COL_W, true);

    ui->cap_trip = ev_caption(ui->scr, EV_COL_L_X, 208, "TRIP A" EV_MIDDOT "KM");
    ui->lb_trip  = ev_label(ui->scr, EV_COL_L_X, 224, "23.6", EV_FONT_NUM_M, EV_INK);

    ui->rule_2 = ev_rule(ui->scr, EV_COL_L_X, EV_RULE_2_Y, EV_COL_W, true);

    ui->cap_motor = ev_caption(ui->scr, EV_COL_L_X, 314, "MOTOR" EV_MIDDOT EV_DEG "C");
    ui->lb_motor  = ev_label(ui->scr, EV_COL_L_X, 330, "42", EV_FONT_NUM_M, EV_INK);

    /*=================================================================
     * RIGHT COLUMN - range, power, cell
     *================================================================*/
    ui->cap_range = ev_caption_r(ui->scr, EV_COL_R_X, 90, EV_COL_W,
                                 "RANGE" EV_MIDDOT "KM");
    ui->lb_range  = ev_label_r(ui->scr, EV_COL_R_X, 108, EV_COL_W, "356",
                               EV_FONT_NUM_L, EV_INK);

    ui->rule_3 = ev_rule(ui->scr, EV_COL_R_X, EV_RULE_1_Y, EV_COL_W, false);

    ui->cap_power = ev_caption_r(ui->scr, EV_COL_R_X, 208, EV_COL_W,
                                 "POWER" EV_MIDDOT "kW");
    ui->lb_power  = ev_label_r(ui->scr, EV_COL_R_X, 224, EV_COL_W, "2.4",
                               EV_FONT_NUM_M, EV_INK);

    /* Symmetrical power bar: fills right of the zero mark under drive,
     * left of it under regeneration. */
    ui->bar_power = lv_bar_create(ui->scr);
    lv_obj_set_pos(ui->bar_power, EV_COL_R_X, 268);
    lv_obj_set_size(ui->bar_power, EV_COL_W, 4);
    lv_bar_set_mode(ui->bar_power, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_range(ui->bar_power, -2000, 6000);
    lv_bar_set_value(ui->bar_power, 2400, LV_ANIM_OFF);
    lv_obj_set_style_anim_time(ui->bar_power, 320, LV_PART_MAIN);
    lv_obj_set_style_radius(ui->bar_power, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->bar_power, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->bar_power, EV_TRACK, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->bar_power, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(ui->bar_power, 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui->bar_power, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui->bar_power, EV_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(ui->bar_power, EV_ICE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(ui->bar_power, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_color(ui->bar_power, EV_BLUE, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_width(ui->bar_power, 12, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_opa(ui->bar_power, LV_OPA_60, LV_PART_INDICATOR);

    /* Zero mark sits where -2000 of a -2000..6000 range falls: 25 % along. */
    ui->power_zero = ev_block(ui->scr,
                              EV_COL_R_X + (EV_COL_W * 2000) / 8000 - 1, 264, 2, 12);
    lv_obj_set_style_bg_opa(ui->power_zero, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->power_zero, lv_color_hex(0x2B3A4E), LV_PART_MAIN);

    ui->lb_regen = ev_caption(ui->scr, EV_COL_R_X, 282, "REGEN");
    ui->lb_drive = ev_caption_r(ui->scr, EV_COL_R_X, 282, EV_COL_W, "DRIVE");

    ui->rule_4 = ev_rule(ui->scr, EV_COL_R_X, EV_RULE_2_Y, EV_COL_W, false);

    ui->cap_cell = ev_caption_r(ui->scr, EV_COL_R_X, 314, EV_COL_W,
                                "CELL" EV_MIDDOT EV_DEG "C");
    ui->lb_cell  = ev_label_r(ui->scr, EV_COL_R_X, 330, EV_COL_W, "31",
                              EV_FONT_NUM_M, EV_INK);

    /*=================================================================
     * FOOTER
     *================================================================*/
    ui->rule_foot = ev_rule(ui->scr, EV_MARGIN, EV_FOOT_Y,
                            EV_W - 2 * EV_MARGIN, false);
    lv_obj_set_style_bg_grad_dir(ui->rule_foot, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->rule_foot, LV_OPA_50, LV_PART_MAIN);

    ui->cap_odo = ev_caption(ui->scr, EV_MARGIN, 430, "ODOMETER");
    ui->lb_odo  = ev_label(ui->scr, EV_MARGIN, 444, "4,812 km", EV_FONT_UI, EV_INK);

    {
        static const char *names[3] = { "STREET", "CRAWL", "RUSH" };
        for (i = 0; i < 3; i++) {
            lv_obj_t *c = ev_block(ui->scr, 252 + i * 102, 432, 92, 30);
            lv_obj_set_style_radius(c, 15, LV_PART_MAIN);
            lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(c, EV_RULE, LV_PART_MAIN);
            lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);

            ui->chip[i] = c;
            ui->chip_lb[i] = lv_label_create(c);
            lv_label_set_text(ui->chip_lb[i], names[i]);
            lv_obj_set_style_text_font(ui->chip_lb[i], EV_FONT_CAP_S, LV_PART_MAIN);
            lv_obj_set_style_text_letter_space(ui->chip_lb[i], 2, LV_PART_MAIN);
            lv_obj_center(ui->chip_lb[i]);
        }
    }

    ui->cap_ride = ev_caption_r(ui->scr, EV_COL_R_X, 430, EV_COL_W, "RIDE TIME");
    ui->lb_ride  = ev_label_r(ui->scr, EV_COL_R_X, 444, EV_COL_W, "00:42",
                              EV_FONT_UI, EV_INK);

    /*=================================================================
     * WIRE-UP
     *================================================================*/
    ev_dash_apply_mode(ui, EV_MODE_STREET);
    ev_events_attach(ui);
    lv_scr_load(ui->scr);
    ev_dash_boot(ui);
}

/**********************
 *   PUBLIC DRIVERS
 **********************/

void ev_dash_anim_speed(ev_ui_t *ui, int32_t target, uint32_t time_ms)
{
    lv_anim_t a;

    if (ui->arc_ring == NULL) return;
    if (target < 0)             target = 0;
    if (target > EV_SPEED_MAX)  target = EV_SPEED_MAX;

    lv_anim_del(ui->arc_ring, anim_speed_cb);

    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->arc_ring);
    lv_anim_set_exec_cb(&a, anim_speed_cb);
    lv_anim_set_values(&a, lv_arc_get_value(ui->arc_ring), target);
    lv_anim_set_time(&a, time_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void ev_dash_apply_mode(ev_ui_t *ui, ev_mode_t mode)
{
    lv_color_t from, to, accent;
    int i;

    switch (mode) {
        case EV_MODE_RUSH:  from = EV_RUSH_FROM;  to = EV_RUSH_TO;  break;
        case EV_MODE_CRAWL: from = EV_CRAWL_FROM; to = EV_CRAWL_TO; break;
        case EV_MODE_STREET:
        default:            from = EV_STREET_FROM; to = EV_STREET_TO; break;
    }
    accent = lv_color_mix(to, from, 150);

    for (i = 0; i < EV_ARC_SEGMENTS; i++) {
#if EV_ARC_SEGMENTS > 1
        lv_opa_t r = (lv_opa_t)((i * 255) / (EV_ARC_SEGMENTS - 1));
#else
        lv_opa_t r = 0;
#endif
        lv_obj_set_style_arc_color(ui->arc_seg[i], lv_color_mix(to, from, r),
                                   LV_PART_INDICATOR);
    }

    /* The halo carries the same light, the crest is the brightest note. */
    for (i = 0; i < 3; i++) {
        lv_obj_set_style_arc_color(ui->halo[i], accent, LV_PART_MAIN);
    }
    lv_obj_set_style_arc_color(ui->halo_crest, to, LV_PART_MAIN);

    lv_obj_set_style_shadow_color(ui->glow_out, accent, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(ui->glow_in, accent, LV_PART_MAIN);

    lv_obj_set_style_bg_color(ui->batt_fill, from, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(ui->batt_fill, to, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->lb_mode_name, accent, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui->lb_link, accent, LV_PART_MAIN);

    lv_obj_set_style_bg_color(ui->bar_power, from, LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(ui->bar_power, to, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_color(ui->bar_power, accent, LV_PART_INDICATOR);

    for (i = 0; i < 3; i++) {
        ev_chip_paint(ui->chip[i], ui->chip_lb[i], (int)mode == i, accent);
    }

    switch (mode) {
        case EV_MODE_RUSH:  lv_label_set_text(ui->lb_mode_name, "RUSH");   break;
        case EV_MODE_CRAWL: lv_label_set_text(ui->lb_mode_name, "CRAWL");  break;
        default:            lv_label_set_text(ui->lb_mode_name, "STREET"); break;
    }
}

void ev_dash_halo_drift(ev_ui_t *ui)
{
    lv_anim_t a;

    /* Three rings at incommensurate periods never repeat the same picture,
     * which is what stops the idle screen looking like a loop. */
    ev_spin(ui->halo[0],    true,  26000, 0);
    ev_spin(ui->halo[1],    false, 19000, 0);
    ev_spin(ui->halo[2],    true,  34000, 0);
    ev_spin(ui->halo_crest, true,  11000, 0);

    /* Middle ring breathes, bloom breathes against it out of phase. */
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->halo[1]);
    lv_anim_set_exec_cb(&a, anim_arc_opa_cb);
    lv_anim_set_values(&a, 55, 150);
    lv_anim_set_time(&a, 2600);
    lv_anim_set_playback_time(&a, 2600);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->glow_in);
    lv_anim_set_exec_cb(&a, anim_shadow_cb);
    lv_anim_set_values(&a, 48, 88);
    lv_anim_set_time(&a, 3400);
    lv_anim_set_playback_time(&a, 3400);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void ev_dash_boot(ev_ui_t *ui)
{
    lv_anim_t a;
    int i;

    /* Chrome first, then the light, then the data columns. */
    ev_fade_in(ui->batt_shell, 400, 0);
    ev_fade_in(ui->batt_cap,   400, 0);
    ev_fade_in(ui->batt_fill,  400, 60);
    ev_fade_in(ui->lb_soc,     400, 60);
    ev_fade_in(ui->lb_mode_name, 400, 110);
    ev_fade_in(ui->lb_beam,      400, 110);
    ev_fade_in(ui->lb_clock,        400, 160);
    ev_fade_in(ui->lb_link,         400, 200);

    ev_fade_in(ui->glow_out, 900, 80);
    ev_fade_in(ui->glow_in,  900, 80);
    for (i = 0; i < 3; i++) ev_fade_in(ui->halo[i], 700, 120 + i * 90);
    ev_fade_in(ui->halo_crest, 700, 300);
    ev_fade_in(ui->arc_ring, 600, 160);
    for (i = 0; i < EV_ARC_SEGMENTS; i++) ev_fade_in(ui->arc_seg[i], 600, 200);
    ev_fade_in(ui->lb_speed,      500, 360);
    ev_fade_in(ui->lb_speed_unit, 500, 420);
    ev_fade_in(ui->lb_scale_min,  500, 420);
    ev_fade_in(ui->lb_scale_max,  500, 420);

    ev_rise_in(ui->cap_nav,     90, 14, 520, 240);
    ev_rise_in(ui->lb_nav_icon, 112, 14, 520, 260);
    ev_rise_in(ui->lb_nav_dist, 108, 14, 520, 260);
    ev_rise_in(ui->lb_nav_street, 166, 14, 520, 290);
    ev_rise_in(ui->cap_range,   90, 14, 520, 240);
    ev_rise_in(ui->lb_range,   108, 14, 520, 260);

    ev_fade_in(ui->rule_1, 600, 330);
    ev_fade_in(ui->rule_3, 600, 330);
    ev_rise_in(ui->cap_trip,  208, 12, 500, 350);
    ev_rise_in(ui->lb_trip,   224, 12, 500, 370);
    ev_rise_in(ui->cap_power, 208, 12, 500, 350);
    ev_rise_in(ui->lb_power,  224, 12, 500, 370);
    ev_fade_in(ui->bar_power,  500, 400);
    ev_fade_in(ui->power_zero, 500, 400);
    ev_fade_in(ui->lb_regen,   500, 420);
    ev_fade_in(ui->lb_drive,   500, 420);

    ev_fade_in(ui->rule_2, 600, 430);
    ev_fade_in(ui->rule_4, 600, 430);
    ev_rise_in(ui->cap_motor, 314, 12, 500, 450);
    ev_rise_in(ui->lb_motor,  330, 12, 500, 470);
    ev_rise_in(ui->cap_cell,  314, 12, 500, 450);
    ev_rise_in(ui->lb_cell,   330, 12, 500, 470);

    ev_fade_in(ui->rule_foot, 600, 520);
    ev_rise_in(ui->cap_odo, 430, 10, 480, 540);
    ev_rise_in(ui->lb_odo,  444, 10, 480, 560);
    ev_rise_in(ui->cap_ride, 430, 10, 480, 540);
    ev_rise_in(ui->lb_ride,  444, 10, 480, 560);
    for (i = 0; i < 3; i++) ev_rise_in(ui->chip[i], 432, 12, 480, 580 + i * 60);

    /* Halo spin-up: two fast decelerating turns, landing back at 0 so the
     * endless drift can take over without a visible jump. */
    for (i = 0; i < 3; i++) {
        lv_anim_init(&a);
        lv_anim_set_var(&a, ui->halo[i]);
        lv_anim_set_exec_cb(&a, anim_rot_cb);
        lv_anim_set_values(&a, 0, 720);
        lv_anim_set_time(&a, 2000 + i * 180);
        lv_anim_set_delay(&a, 120);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->halo_crest);
    lv_anim_set_exec_cb(&a, anim_rot_cb);
    lv_anim_set_values(&a, 0, 1080);
    lv_anim_set_time(&a, 2300);
    lv_anim_set_delay(&a, 120);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    /* Full-scale sweep of the ring and the numeral. */
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->arc_ring);
    lv_anim_set_exec_cb(&a, anim_speed_cb);
    lv_anim_set_values(&a, 0, EV_SPEED_MAX);
    lv_anim_set_time(&a, 950);
    lv_anim_set_playback_time(&a, 800);
    lv_anim_set_delay(&a, 420);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    /* Hand over to the endless drift once the spin-up has bled off. */
    lv_timer_t *t = lv_timer_create(drift_handover_cb, 2450, ui);
    lv_timer_set_repeat_count(t, 1);
}

static void drift_handover_cb(lv_timer_t *t)
{
    ev_dash_halo_drift((ev_ui_t *)t->user_data);
}

/**********************
 *   ANIMATION CBs
 **********************/
static void anim_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

static void anim_arc_opa_cb(void *var, int32_t v)
{
    lv_obj_set_style_arc_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_MAIN);
}

static void anim_y_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, (lv_coord_t)v);
}

static void anim_rot_cb(void *var, int32_t v)
{
    lv_arc_set_rotation((lv_obj_t *)var, (uint16_t)(((v % 360) + 360) % 360));
}

static void anim_shadow_cb(void *var, int32_t v)
{
    lv_obj_set_style_shadow_width((lv_obj_t *)var, (lv_coord_t)v, LV_PART_MAIN);
}

/* One animation drives the ring value, every gradient segment and the
 * numeral, so they can never disagree. */
static void anim_speed_cb(void *var, int32_t v)
{
    const int32_t step = EV_SPEED_MAX / EV_ARC_SEGMENTS;
    int i;

    lv_arc_set_value((lv_obj_t *)var, v);
    if (s_ui == NULL) return;

    for (i = 0; i < EV_ARC_SEGMENTS; i++) {
        int32_t local = v - (int32_t)i * step;
        if (local < 0)    local = 0;
        if (local > step) local = step;
        lv_arc_set_value(s_ui->arc_seg[i], local);
    }
    lv_label_set_text_fmt(s_ui->lb_speed, "%d", (int)v);
}

/**********************
 *   ANIMATION HELPERS
 **********************/
static void ev_fade_in(lv_obj_t *obj, uint32_t time, uint32_t delay)
{
    lv_anim_t a;

    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_opa_cb);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void ev_rise_in(lv_obj_t *obj, lv_coord_t y_end, lv_coord_t dy,
                       uint32_t time, uint32_t delay)
{
    lv_anim_t a;

    ev_fade_in(obj, time, delay);
    lv_obj_set_y(obj, (lv_coord_t)(y_end + dy));

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_values(&a, y_end + dy, y_end);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void ev_spin(lv_obj_t *obj, bool cw, uint32_t period, uint32_t delay)
{
    lv_anim_t a;

    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_rot_cb);
    lv_anim_set_values(&a, cw ? 0 : 360, cw ? 360 : 0);
    lv_anim_set_time(&a, period);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
}

/**********************
 *   BUILD HELPERS
 **********************/
static lv_obj_t *ev_label(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                          const char *txt, const lv_font_t *font,
                          lv_color_t col)
{
    lv_obj_t *l = lv_label_create(par);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, col, LV_PART_MAIN);
    return l;
}

static lv_obj_t *ev_label_r(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                            lv_coord_t w, const char *txt,
                            const lv_font_t *font, lv_color_t col)
{
    lv_obj_t *l = ev_label(par, x, y, txt, font, col);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    return l;
}

static lv_obj_t *ev_caption(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                            const char *txt)
{
    lv_obj_t *l = ev_label(par, x, y, txt, EV_FONT_CAP_S, EV_INK_MUTE);
    lv_obj_set_style_text_letter_space(l, 2, LV_PART_MAIN);
    return l;
}

static lv_obj_t *ev_caption_r(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, const char *txt)
{
    lv_obj_t *l = ev_caption(par, x, y, txt);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    return l;
}

/* Plain rectangle with every default stripped - the base for hairlines,
 * ticks, the battery glyph and the chips. */
static lv_obj_t *ev_block(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                          lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *o = lv_obj_create(par);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_radius(o, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(o, 0, LV_PART_MAIN);
    return o;
}

/* 1 px rule.  `fade_right` decides which way it dissolves, so the two
 * columns fade away from the centre rather than both to the right. */
static lv_obj_t *ev_rule(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                         lv_coord_t w, bool fade_right)
{
    lv_obj_t *r = ev_block(par, x, y, w, 1);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(r, fade_right ? EV_RULE : EV_BLACK, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(r, fade_right ? EV_BLACK : EV_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(r, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    return r;
}

/* Soft bloom: a circle with no fill whose shadow is the only thing drawn. */
static lv_obj_t *ev_glow(lv_obj_t *par, lv_coord_t size, lv_coord_t blur,
                         lv_coord_t spread, lv_opa_t opa)
{
    lv_obj_t *g = ev_block(par, EV_CENTER_X(size), EV_CENTER_Y(size), size, size);
    lv_obj_set_style_radius(g, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(g, EV_BLUE, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g, blur, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(g, spread, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g, opa, LV_PART_MAIN);
    return g;
}

/* Decorative ring: only the background arc is drawn, and `span` degrees of
 * it, so rotating the widget sweeps a light around the dial. */
static lv_obj_t *ev_halo_ring(lv_obj_t *par, lv_coord_t size, lv_coord_t w,
                              uint16_t span, lv_opa_t opa)
{
    lv_obj_t *r = lv_arc_create(par);

    lv_obj_set_pos(r, EV_CENTER_X(size), EV_CENTER_Y(size));
    lv_obj_set_size(r, size, size);
    lv_obj_remove_style(r, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(r, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(r, 0, LV_PART_MAIN);
    lv_arc_set_bg_angles(r, 0, span);
    lv_arc_set_rotation(r, 0);
    lv_arc_set_range(r, 0, 100);
    lv_arc_set_value(r, 0);
    lv_obj_set_style_arc_width(r, w, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(r, true, LV_PART_MAIN);
    lv_obj_set_style_arc_color(r, EV_BLUE, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(r, opa, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(r, LV_OPA_TRANSP, LV_PART_INDICATOR);
    return r;
}

static void ev_chip_paint(lv_obj_t *chip, lv_obj_t *label, bool on,
                          lv_color_t accent)
{
    if (on) {
        lv_obj_set_style_bg_opa(chip, 40, LV_PART_MAIN);
        lv_obj_set_style_bg_color(chip, accent, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(chip, EV_BLACK, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(chip, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_border_color(chip, accent, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(chip, accent, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(chip, 20, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(chip, LV_OPA_40, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, EV_INK, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(chip, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(chip, LV_GRAD_DIR_NONE, LV_PART_MAIN);
        lv_obj_set_style_border_color(chip, EV_RULE, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(chip, 0, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, EV_INK_MUTE, LV_PART_MAIN);
    }
}
