/*
 * cl_screen.c
 *
 * Construction of the cluster screen, in the order the frame layers it:
 * the pool of light at the back, then the two rings and the stadium
 * outline, then the status strip, then everything that carries a number.
 *
 * The layout is absolute.  A cluster is a fixed panel, the design is drawn
 * at exactly 800 x 480, and a flex or grid pass here would only be a
 * roundabout way of arriving back at the same coordinates - so every
 * position is the Figma node's own x/y, named in cl_theme.h.
 *
 * Three things the design asks for have no direct LVGL equivalent, and each
 * is handled where it appears below: the radial pool of light (a computed
 * alpha map, see cl_pool.c), the stadium outline (two arcs and a hairline)
 * and the mode pill's inset glow (a baked alpha map).
 */

#include "cl_screen.h"
#include "cl_pool.h"

#include <stdio.h>

cl_screen_t cl_ui;

/*--------------------------------------------------------------------------
 *  Small builders
 *
 *  LVGL's default theme puts padding, a background and a border on every
 *  object.  Nothing in this design wants any of that, so everything starts
 *  from a cleared style and opts back in.
 *-------------------------------------------------------------------------*/

static lv_obj_t *bare(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *o = lv_obj_create(par);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    return o;
}

/* A block of colour: the hairlines and the range panel are both this. */
static lv_obj_t *fill(lv_obj_t *par, lv_coord_t x, lv_coord_t y,
                      lv_coord_t w, lv_coord_t h, lv_color_t c, lv_opa_t opa)
{
    lv_obj_t *o = bare(par, x, y, w, h);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    return o;
}

/* Text placed by the top-left of its line box, the way Figma reports it. */
static lv_obj_t *text(lv_obj_t *par, const cl_face_t *face, lv_color_t c,
                      lv_opa_t opa, lv_coord_t x, lv_coord_t y, const char *s)
{
    lv_obj_t *l = lv_label_create(par);
    lv_obj_remove_style_all(l);
    lv_obj_set_style_text_font(l, face->font, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_obj_set_style_text_opa(l, opa, 0);
    lv_label_set_text(l, s);
    lv_obj_set_pos(l, x, y + face->lead);
    return l;
}

/* Text that has to stay centred on a point as its content changes - the
 * odometer and the two AUTO BALANCE lines.  A fixed box wide enough for the
 * longest value, centred on the design's centre line, so the label never
 * has to be re-measured when the value updates. */
static lv_obj_t *text_centred(lv_obj_t *par, const cl_face_t *face,
                              lv_color_t c, lv_opa_t opa, lv_coord_t cx,
                              lv_coord_t y, lv_coord_t w, const char *s)
{
    lv_obj_t *l = text(par, face, c, opa, cx - w / 2, y, s);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    return l;
}

/* Text pinned by its right edge - only the clock needs it. */
static lv_obj_t *text_right(lv_obj_t *par, const cl_face_t *face, lv_color_t c,
                            lv_opa_t opa, lv_coord_t right, lv_coord_t y,
                            lv_coord_t w, const char *s)
{
    lv_obj_t *l = text(par, face, c, opa, right - w, y, s);
    lv_obj_set_width(l, w);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_RIGHT, 0);
    return l;
}

/* An alpha map tinted at draw time.  All the icons arrive this way, which is
 * what lets a tell-tale change colour without a second bitmap. */
static lv_obj_t *icon(lv_obj_t *par, const lv_img_dsc_t *src, lv_color_t c,
                      lv_opa_t opa, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *i = lv_img_create(par);
    lv_obj_remove_style_all(i);
    lv_img_set_src(i, src);
    lv_obj_set_style_img_recolor(i, c, 0);
    lv_obj_set_style_img_recolor_opa(i, LV_OPA_COVER, 0);
    lv_obj_set_style_img_opa(i, opa, 0);
    lv_obj_set_pos(i, x, y);
    return i;
}

/* One of the two 290px rings, showing only the half that faces the dial.
 * `from`/`to` are LVGL arc angles: zero at three o'clock, running clockwise. */
static lv_obj_t *ring(lv_obj_t *par, lv_coord_t x, lv_coord_t y, lv_coord_t d,
                      uint16_t from, uint16_t to)
{
    lv_obj_t *a = lv_arc_create(par);
    lv_obj_remove_style_all(a);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(a, x, y);
    lv_obj_set_size(a, d, d);
    lv_arc_set_bg_angles(a, from, to);
    lv_arc_set_rotation(a, 0);
    lv_obj_set_style_arc_color(a, CL_WHITE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 1, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(a, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 0, LV_PART_INDICATOR);
    return a;
}

/* A white pinpoint with a soft halo - the two dots at nodes 1:21 and 1:22.
 * Figma draws these as a 12px circle plus a drop shadow dilated by 5, offset
 * down 4 and blurred hard; LVGL spells the same thing as shadow_spread,
 * shadow_ofs_y and shadow_width.  The halo is broad and faint - it is still
 * a couple of percent thirty pixels out - so the width is set from where it
 * dies in the frame rather than from Figma's blur radius, which is not the
 * same measure. */
static lv_obj_t *glow_dot(lv_obj_t *par, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *o = bare(par, x, y, CL_DOT_D, CL_DOT_D);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, CL_WHITE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(o, CL_WHITE, 0);
    lv_obj_set_style_shadow_opa(o, 255, 0);
    lv_obj_set_style_shadow_width(o, 90, 0);
    lv_obj_set_style_shadow_spread(o, 5, 0);
    lv_obj_set_style_shadow_ofs_y(o, 4, 0);
    return o;
}

/*--------------------------------------------------------------------------
 *  The pool of light
 *
 *  Node 1:3 fills the frame with a radial gradient - a navy core roughly 230
 *  wide and 140 tall fading to nothing by the edges - and LVGL 8 has only
 *  linear ones.
 *
 *  Figma states the gradient exactly, so it is evaluated rather than
 *  approximated - see cl_pool.c, which also records the two approaches that
 *  do not work.  By the time it gets here it is an ordinary alpha map the
 *  size of the screen, tinted with the core colour like any other icon.
 *-------------------------------------------------------------------------*/

static void pool_of_light(lv_obj_t *par)
{
    cl_pool_build();
    icon(par, &cl_pool, CL_POOL, LV_OPA_COVER, 0, 0);
}

/*--------------------------------------------------------------------------
 *  The stadium outline
 *
 *  Node 1:3's radius is larger than half its height, so the shape resolves
 *  to a stadium and only its top stroke is drawn - all the way round both
 *  end caps, because the sides have no stroke of their own to hand the
 *  corner over to.
 *
 *  LVGL's LV_BORDER_SIDE_TOP cannot do this: it pushes the inner rectangle
 *  outwards on the three unset sides, which leaves the curved part outside
 *  the border band and squares the shape off.  Two arcs and a hairline give
 *  the real geometry.
 *-------------------------------------------------------------------------*/

static void stadium_outline(lv_obj_t *par)
{
    lv_coord_t d = CL_BACK_R * 2;

    /* Left cap: nine o'clock round to twelve. */
    ring(par, CL_BACK_L_CX - CL_BACK_R, CL_BACK_Y, d, 180, 270);
    /* Right cap: twelve round to three. */
    ring(par, CL_BACK_R_CX - CL_BACK_R, CL_BACK_Y, d, 270, 360);

    /* ...and the flat run between the two tangent points. */
    fill(par, CL_BACK_L_CX, CL_BACK_Y, CL_BACK_R_CX - CL_BACK_L_CX, 1,
         CL_WHITE, CL_OPA_TELLTALE);

    /* The caps are the same hairline at the same opacity; LVGL antialiases
     * them, so they fade out towards the sides exactly as the frame does. */
    lv_obj_set_style_arc_opa(lv_obj_get_child(par, -3), CL_OPA_TELLTALE,
                             LV_PART_MAIN);
    lv_obj_set_style_arc_opa(lv_obj_get_child(par, -2), CL_OPA_TELLTALE,
                             LV_PART_MAIN);
}

/*--------------------------------------------------------------------------
 *  The AUTO BALANCE curve
 *
 *  Node 1:23 is a cubic with a 300px chord and a 25.5px rise.  A circular
 *  arc through the same three points drifts a couple of pixels at the
 *  quarter marks, so the real curve is flattened here and drawn as a
 *  polyline.  The points have to outlive this function - lv_line keeps the
 *  array by reference rather than copying it.
 *-------------------------------------------------------------------------*/

static lv_point_t curve_pts[CL_CURVE_STEPS + 1];

static void balance_curve(lv_obj_t *par)
{
    lv_obj_t *line;
    int i;

    for (i = 0; i <= CL_CURVE_STEPS; i++) {
        float t = (float)i / CL_CURVE_STEPS;
        float u = 1.0f - t;
        float a = u * u * u,     b = 3.0f * u * u * t;
        float c = 3.0f * u * t * t, d = t * t * t;

        curve_pts[i].x = (lv_coord_t)(a * CL_CURVE_P0_X + b * CL_CURVE_P1_X +
                                      c * CL_CURVE_P2_X + d * CL_CURVE_P3_X + 0.5f);
        curve_pts[i].y = (lv_coord_t)(a * CL_CURVE_P0_Y + b * CL_CURVE_P1_Y +
                                      c * CL_CURVE_P2_Y + d * CL_CURVE_P3_Y + 0.5f);
    }

    /* The node also carries a blurred white copy behind the stroke, but
     * against this background it measures as nothing at all - the frame is
     * pure backdrop a single pixel either side of the line - so there is no
     * glow layer here.
     *
     * Square joins rather than round ones, despite the node asking for round
     * caps: lv_line blends each segment separately, so round caps overlap at
     * every joint and blend twice, which turns an 80% stroke into a 93% one
     * wherever two segments meet.  At 1.6 degrees of turn per joint a square
     * join leaves a gap of about three hundredths of a pixel. */
    line = lv_line_create(par);
    lv_obj_remove_style_all(line);
    lv_line_set_points(line, curve_pts, CL_CURVE_STEPS + 1);
    lv_obj_set_pos(line, 0, 0);
    lv_obj_set_style_line_color(line, CL_WHITE, 0);
    lv_obj_set_style_line_opa(line, CL_OPA_PRIMARY, 0);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, false, 0);
}

/*--------------------------------------------------------------------------
 *  Sections
 *-------------------------------------------------------------------------*/

static void status_strip(cl_screen_t *ui, lv_obj_t *par)
{
    ui->clock = text_right(par, &CL_SEMI_14, CL_WHITE, CL_OPA_SECONDARY,
                           CL_CLOCK_R, CL_CLOCK_Y, 90, "05:35 PM");

    fill(par, CL_TICK_X, CL_TICK_Y, 1, CL_TICK_H, CL_WHITE, 102);

    ui->soc = text(par, &CL_SEMI_14, CL_WHITE, CL_OPA_SECONDARY,
                   CL_SOC_X, CL_SOC_Y, "89%");
    ui->battery = icon(par, &cl_icon_battery, CL_BATTERY, LV_OPA_COVER,
                       CL_BATT_X, CL_BATT_Y);

    ui->nav_led = icon(par, &cl_icon_navigation, CL_NAV, LV_OPA_COVER,
                       CL_NAVLED_X, CL_NAVLED_Y);
    ui->bluetooth = icon(par, &cl_icon_bluetooth, CL_LINK, CL_OPA_LINK,
                         CL_BT_X, CL_BT_Y);
    ui->signal = text(par, &CL_SEMI_18, CL_WHITE, LV_OPA_COVER,
                      CL_SIGNAL_X, CL_SIGNAL_Y, "4G");
}

static void tell_tales(cl_screen_t *ui, lv_obj_t *par)
{
    /* Node 1:18 is a blurred orange disc sitting behind the warning lamp.
     * Drawn as a small circle whose shadow does all the work. */
    ui->warning_halo = bare(par, CL_HALO_X, CL_HALO_Y, CL_HALO_D, CL_HALO_D);
    lv_obj_set_style_radius(ui->warning_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui->warning_halo, CL_WARN_HALO, 0);
    lv_obj_set_style_bg_opa(ui->warning_halo, 230, 0);
    lv_obj_set_style_shadow_color(ui->warning_halo, CL_WARN_HALO, 0);
    lv_obj_set_style_shadow_opa(ui->warning_halo, 255, 0);
    lv_obj_set_style_shadow_width(ui->warning_halo, 30, 0);

    ui->warning = icon(par, &cl_icon_warning, CL_WHITE, LV_OPA_COVER,
                       CL_WARN_X, CL_WARN_Y);
    ui->beam = icon(par, &cl_icon_headlight, CL_WHITE, CL_OPA_TELLTALE,
                    CL_BEAM_X, CL_BEAM_Y);
    ui->italic = icon(par, &cl_icon_italic_i, CL_WHITE, CL_OPA_TELLTALE,
                      CL_ITALIC_X, CL_ITALIC_Y);
}

static void navigation(cl_screen_t *ui, lv_obj_t *par)
{
    ui->disc = bare(par, CL_DISC_X, CL_DISC_Y, CL_DISC_D, CL_DISC_D);
    lv_obj_set_style_radius(ui->disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui->disc, CL_WHITE, 0);
    lv_obj_set_style_bg_opa(ui->disc, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_color(ui->disc, lv_color_hex(0x020A0F), 0);
    lv_obj_set_style_shadow_opa(ui->disc, 128, 0);
    lv_obj_set_style_shadow_width(ui->disc, 12, 0);
    lv_obj_set_style_shadow_spread(ui->disc, 2, 0);
    lv_obj_set_style_shadow_ofs_y(ui->disc, 2, 0);

    ui->manoeuvre = icon(par, &cl_icon_manoeuvre, CL_BLACK, LV_OPA_COVER,
                         CL_MANOEUVRE_X, CL_MANOEUVRE_Y);

    ui->street = text(par, &CL_SEMI_20, CL_WHITE, CL_OPA_PRIMARY,
                      CL_STREET_X, CL_STREET_Y, "CARTER ROAD");
    ui->distance = text(par, &CL_SEMI_18, CL_WHITE, CL_OPA_PRIMARY,
                        CL_DIST_X, CL_DIST_Y, "200m away");
}

static void speed_readout(cl_screen_t *ui, lv_obj_t *par)
{
    ui->turn_left = icon(par, &cl_icon_turn_left, CL_TURN, LV_OPA_COVER,
                         CL_TURN_L_X, CL_TURN_Y);
    ui->turn_right = icon(par, &cl_icon_turn_right, CL_TURN, LV_OPA_COVER,
                          CL_TURN_R_X, CL_TURN_Y);

    /* The numeral is centred on the dial rather than pinned left, so 9 and
     * 150 sit on the same axis instead of drifting off it. */
    ui->speed = text_centred(par, &CL_SPEED_136, CL_WHITE, LV_OPA_COVER,
                             CL_W / 2, CL_SPEED_Y, 320, "56");
    text(par, &CL_BOLD_16, CL_WHITE, LV_OPA_COVER,
         CL_UNIT_X, CL_UNIT_Y, "KM/HR");
}

static void mode_selector(cl_screen_t *ui, lv_obj_t *par)
{
    lv_obj_t *pill;

    glow_dot(par, CL_MODE_DOT_X, CL_MODE_DOT_Y);

    ui->mode_prev = text(par, &CL_SEMI_20, CL_WHITE, LV_OPA_COVER,
                         CL_MODE_X, CL_MODE_PREV_Y, "CRAWL");
    ui->mode_next = text(par, &CL_SEMI_20, CL_WHITE, LV_OPA_COVER,
                         CL_MODE_X, CL_MODE_NEXT_Y, "RUSH");

    pill = bare(par, CL_PILL_X, CL_PILL_Y, CL_PILL_W, CL_PILL_H);
    lv_obj_set_style_radius(pill, CL_PILL_R, 0);
    lv_obj_set_style_bg_color(pill, lv_color_hex(0xD9D9D9), 0);
    lv_obj_set_style_bg_opa(pill, 13, 0);            /* the frame's 5%      */
    lv_obj_set_style_border_color(pill, CL_WHITE, 0);
    lv_obj_set_style_border_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pill, CL_PILL_BORDER, 0);

    /* The pill's inset shadow, baked to an alpha map because LVGL's shadows
     * only cast outwards.  It is exactly the size of the padding box, so it
     * drops straight inside the border. */
    icon(par, &cl_glow_pill, CL_PILL_GLOW, LV_OPA_COVER,
         CL_PILL_X + CL_PILL_BORDER, CL_PILL_Y + CL_PILL_BORDER);

    ui->mode_curr = text_centred(par, &CL_SEMI_24, CL_WHITE, LV_OPA_COVER,
                                 CL_PILL_X + CL_PILL_W / 2, CL_MODE_CURR_Y,
                                 CL_PILL_W, "STREET");
}

static void right_column(cl_screen_t *ui, lv_obj_t *par)
{
    lv_obj_t *panel;

    ui->odometer = text_centred(par, &CL_SEMI_24, CL_WHITE, LV_OPA_COVER,
                                CL_ODO_CX, CL_ODO_Y, 180, "8999999");
    text(par, &CL_SEMI_16, CL_WHITE, CL_OPA_SECONDARY,
         CL_ODO_CAP_X, CL_ODO_CAP_Y, "TOTAL KM");

    /* Node 1:45: a flat light panel with a dark inset shadow along its top
     * edge.  A vertical bg gradient nearly does it, but a blurred edge is an
     * S curve and a two-stop gradient is a straight line, so the two part
     * company by about seventeen counts halfway down the ramp.  The shade is
     * baked the same way the pill's glow is and laid over the flat panel. */
    panel = bare(par, CL_RANGE_X, CL_RANGE_Y, CL_RANGE_W, CL_RANGE_H);
    lv_obj_set_style_radius(panel, CL_RANGE_R, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(panel, CL_PANEL, 0);

    icon(par, &cl_shade_panel, CL_BLACK, LV_OPA_COVER,
         CL_RANGE_X, CL_RANGE_Y);

    text(par, &CL_RANGE_15, CL_BLACK, CL_OPA_SECONDARY,
         CL_RANGE_CAP_X, CL_RANGE_CAP_Y, "RANGE");
    ui->range = text(par, &CL_SEMI_24, CL_BLACK, LV_OPA_COVER,
                     CL_RANGE_VAL_X, CL_RANGE_VAL_Y, "72 KM");
}

static void auto_balance(cl_screen_t *ui, lv_obj_t *par)
{
    balance_curve(par);
    glow_dot(par, CL_CURVE_DOT_X, CL_CURVE_DOT_Y);

    ui->ab_title = text_centred(par, &CL_BOLD_22, CL_INK_SOFT,
                                LV_OPA_COVER, CL_AB_CX, CL_AB_Y, 300,
                                "AUTO BALANCE");
    ui->ab_hint = text_centred(par, &CL_BOLD_14, CL_INK_SOFT,
                               CL_OPA_SECONDARY, CL_AB_HINT_CX, CL_AB_HINT_Y,
                               300, "Press AB to turn on");
}

/*--------------------------------------------------------------------------
 *  Public
 *-------------------------------------------------------------------------*/

const char *cl_mode_name(cl_mode_t mode)
{
    switch (mode) {
        case CL_MODE_CRAWL:  return "CRAWL";
        case CL_MODE_STREET: return "STREET";
        case CL_MODE_RUSH:   return "RUSH";
        default:             return "";
    }
}

void cl_screen_set_mode(cl_screen_t *ui, cl_mode_t mode)
{
    int m = (int)mode % CL_MODE_COUNT;

    lv_label_set_text(ui->mode_prev,
                      cl_mode_name((cl_mode_t)((m + CL_MODE_COUNT - 1) % CL_MODE_COUNT)));
    lv_label_set_text(ui->mode_curr, cl_mode_name((cl_mode_t)m));
    lv_label_set_text(ui->mode_next,
                      cl_mode_name((cl_mode_t)((m + 1) % CL_MODE_COUNT)));
}

void cl_screen_create(cl_screen_t *ui)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_t *bezel;

    lv_obj_remove_style_all(scr);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(scr, CL_W, CL_H);
    lv_obj_set_style_bg_color(scr, CL_BLACK, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui->screen = scr;

    pool_of_light(scr);
    stadium_outline(scr);

    /* The two rings sit above the pool but below everything that carries
     * information - nodes 1:4 and 1:5, each showing its inward-facing half. */
    ring(scr, CL_RING_L_X, CL_RING_Y, CL_RING_D, 270, 90);
    ring(scr, CL_RING_R_X, CL_RING_Y, CL_RING_D, 90, 270);

    status_strip(ui, scr);
    tell_tales(ui, scr);
    navigation(ui, scr);
    speed_readout(ui, scr);
    mode_selector(ui, scr);
    right_column(ui, scr);
    auto_balance(ui, scr);

    /* Node 1:2's own outline, and the reason it is a separate object on top
     * rather than a border on the screen: a border shrinks an object's
     * content area, which would push every absolute coordinate above one
     * pixel down and right. */
    bezel = bare(scr, 0, 0, CL_W, CL_H);
    lv_obj_set_style_radius(bezel, CL_FRAME_RADIUS, 0);
    lv_obj_set_style_bg_opa(bezel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(bezel, CL_BEZEL, 0);
    lv_obj_set_style_border_opa(bezel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bezel, 1, 0);

    lv_scr_load(scr);
}
