/*
 * ui_theme.h
 *
 * Design tokens.  Everything visual is here - change a value and the whole
 * cluster follows.
 *
 * The palette is deliberately cold: pure black ground, a single electric
 * blue that carries the light, and cool greys for type.  Nothing is warm
 * except the two states that must read as warnings.
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_fonts.h"

/**********************
 *      GEOMETRY
 **********************/
#define EV_W                800
#define EV_H                480

#define EV_MARGIN           24                      /* outer safe area     */
#define EV_COL_W            208                     /* side column width   */
#define EV_COL_L_X          EV_MARGIN               /* left column         */
#define EV_COL_R_X          (EV_W - EV_MARGIN - EV_COL_W)

/* Shared horizontal rules - both columns hang off these two lines. */
#define EV_RULE_1_Y         194
#define EV_RULE_2_Y         300

#define EV_TOPBAR_H         46
#define EV_FOOT_Y           418

/* Speed dial */
#define EV_DIAL_CX          400
#define EV_DIAL_CY          236
#define EV_ARC_SIZE         300                     /* outer speed ring    */
#define EV_ARC_W            8                       /* ring thickness      */
#define EV_ARC_SEGMENTS     6                       /* gradient resolution */
#define EV_SPEED_MAX        120

/* Decorative halo rings, inside the speed ring */
#define EV_HALO_1           252
#define EV_HALO_2           232
#define EV_HALO_3           208
#define EV_HALO_CREST       242                     /* the bright crescent */
#define EV_GLOW_OUT         292
#define EV_GLOW_IN          214

#define EV_CENTER_X(s)      (EV_DIAL_CX - (s) / 2)
#define EV_CENTER_Y(s)      (EV_DIAL_CY - (s) / 2)

/**********************
 *       COLOURS
 **********************/
#define EV_BLACK            lv_color_hex(0x000000)  /* the ground          */
#define EV_INK              lv_color_hex(0xF0F5FF)  /* primary type        */
#define EV_INK_DIM          lv_color_hex(0x8496AE)  /* secondary type      */
#define EV_INK_MUTE         lv_color_hex(0x4C5A70)  /* captions            */
#define EV_RULE             lv_color_hex(0x18212F)  /* hairlines           */
#define EV_TRACK            lv_color_hex(0x18222F)  /* unfilled ring / bar */

#define EV_BLUE             lv_color_hex(0x4D8DFF)  /* the light           */
#define EV_BLUE_DEEP        lv_color_hex(0x1B49C4)
#define EV_CYAN             lv_color_hex(0x7FD8FF)
#define EV_ICE              lv_color_hex(0xCFE4FF)  /* near-white blue     */
#define EV_TEAL             lv_color_hex(0x35D6C0)
#define EV_VIOLET           lv_color_hex(0xA66BFF)
#define EV_PINK             lv_color_hex(0xFF6FD8)

#define EV_AMBER            lv_color_hex(0xFFB020)  /* caution             */
#define EV_RED              lv_color_hex(0xFF5A6E)  /* fault               */
#define EV_GREEN            lv_color_hex(0x3DDC84)  /* regen, indicators   */

/**********************
 *      RIDE MODES
 **********************/
typedef enum {
    EV_MODE_STREET = 0,
    EV_MODE_CRAWL  = 1,
    EV_MODE_RUSH   = 2,
} ev_mode_t;

/* Every mode owns the gradient painted along the speed ring; the midpoint
 * of the pair becomes the accent for the halo, glow and tell-tales. */
#define EV_STREET_FROM      EV_BLUE_DEEP
#define EV_STREET_TO        EV_ICE
#define EV_CRAWL_FROM       lv_color_hex(0x0E7C8C)
#define EV_CRAWL_TO         EV_TEAL
#define EV_RUSH_FROM        lv_color_hex(0x5B3BD6)
#define EV_RUSH_TO          EV_PINK

/**********************
 *   NAV MANOEUVRES
 **********************/
typedef enum {
    EV_NAV_STRAIGHT = 0,
    EV_NAV_LEFT     = 1,
    EV_NAV_RIGHT    = 2,
    EV_NAV_UTURN    = 3,
} ev_nav_t;

/**********************
 *       STRINGS
 **********************/
#define EV_DEG              "\xC2\xB0"              /* U+00B0             */
#define EV_MIDDOT           " \xC2\xB7 "            /* U+00B7 with spaces */

#ifdef __cplusplus
}
#endif

#endif /* UI_THEME_H */
