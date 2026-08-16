/*
 * cl_theme.h
 *
 * Design tokens for the cluster, traced one-to-one from the Figma frame
 * "Dashboard - v2" (file u7aoLFxLjCtp1zq2fgMUdD, node 1:2).
 *
 * Every geometry constant carries the Figma node id it came from, so a value
 * that looks arbitrary can be checked against the source of truth instead of
 * guessed at.  Coordinates are absolute within the 800 x 480 frame and are
 * the node's own x/y - not the exported CSS, which rounds a pixel off in
 * places.  Where the two disagreed the rendered frame was measured and the
 * node values won.
 *
 * The palette is nearly monochrome: a dark navy pool of light behind the
 * middle of the screen, white type over it, and exactly four saturated
 * colours reserved for things that must be read at a glance - the turn
 * indicators, the battery, the navigation arrow and the link status.
 */

#ifndef CL_THEME_H
#define CL_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "cl_fonts.h"
#include "cl_icons.h"

/**********************
 *       CANVAS
 **********************/
#define CL_W                    800
#define CL_H                    480
#define CL_FRAME_RADIUS         6                   /* node 1:2            */

/**********************
 *       COLOURS
 **********************/
#define CL_BLACK                lv_color_hex(0x000000)
#define CL_BEZEL                lv_color_hex(0x292929) /* node 1:2 border  */
#define CL_WHITE                lv_color_hex(0xFFFFFF)

#define CL_POOL                 lv_color_hex(0x103557) /* node 1:3 core    */
#define CL_INK_SOFT             lv_color_hex(0xE3F0F7) /* nodes 1:16, 1:17 */

#define CL_PILL_GLOW            lv_color_hex(0x0095FF) /* node 1:32        */
#define CL_PANEL                lv_color_hex(0xD9D9D9) /* node 1:45        */
#define CL_PANEL_SHADE          lv_color_hex(0x6E6E6E) /* its inset shadow */

#define CL_TURN                 lv_color_hex(0x00FF22) /* nodes 1:10, 1:12 */
#define CL_BATTERY              lv_color_hex(0x17FF01) /* node 1:48        */
#define CL_NAV                  lv_color_hex(0x0EC491) /* node 1:39        */
#define CL_LINK                 lv_color_hex(0x258BEB) /* node 1:37        */
#define CL_WARN_HALO            lv_color_hex(0xD0831F) /* node 1:18        */

/* Opacities the frame uses often enough to deserve a name. */
#define CL_OPA_TELLTALE         128     /* white 0.5  - dormant tell-tales */
#define CL_OPA_SECONDARY        153     /* white 0.6  - captions           */
#define CL_OPA_PRIMARY          204     /* white 0.8  - nav text, curve    */
#define CL_OPA_LINK             204     /* the bluetooth glyph is 0.8      */

/**********************
 *      BACKDROP
 **********************/
/* Node 1:3 is an 800x477 rectangle with a 240px radius, which the renderer
 * clamps to half the height - so the shape is a stadium: a flat top between
 * the two tangent points and a semicircular cap at each end.  Only its top
 * stroke is drawn, and it runs the full way round both caps.  LVGL cannot
 * express "top border only" on a radius this large (it squares the corners
 * off), so the outline is assembled from two arcs and one hairline. */
/* The backdrop gradient is baked at a quarter scale and drawn back up; keep
 * this in step with POOL_SCALE in icons/generate.py. */
#define CL_POOL_SCALE           4

#define CL_BACK_Y               3
#define CL_BACK_H               477
#define CL_BACK_R               (CL_BACK_H / 2)         /* 238             */
#define CL_BACK_CY              (CL_BACK_Y + CL_BACK_R) /* 241             */
#define CL_BACK_L_CX            CL_BACK_R               /* 238             */
#define CL_BACK_R_CX            (CL_W - CL_BACK_R)      /* 562             */

/* The two big rings either side of the dial: nodes 1:4 and 1:5, each a
 * 290px circle showing only the half that faces the middle of the screen. */
#define CL_RING_D               290
#define CL_RING_Y               97
#define CL_RING_L_X             (-46)
#define CL_RING_R_X             547

/**********************
 *     STATUS STRIP
 **********************/
#define CL_CLOCK_R              387     /* node 1:34, right-aligned        */
#define CL_CLOCK_Y              9
#define CL_TICK_X               400     /* node 1:42                       */
#define CL_TICK_Y               3
#define CL_TICK_H               27
#define CL_SOC_X                413     /* node 1:41                       */
#define CL_SOC_Y                9
#define CL_BATT_X               451     /* node 1:48                       */
#define CL_BATT_Y               6
#define CL_NAVLED_X             699     /* node 1:39                       */
#define CL_NAVLED_Y             9
#define CL_BT_X                 734     /* node 1:37                       */
#define CL_BT_Y                 35
#define CL_SIGNAL_X             762     /* node 1:35                       */
#define CL_SIGNAL_Y             75

/**********************
 *      TELL-TALES
 **********************/
#define CL_WARN_X               76      /* node 1:19                       */
#define CL_WARN_Y               10
#define CL_HALO_X               78      /* node 1:18, behind the warning   */
#define CL_HALO_Y               12
#define CL_HALO_D               24
#define CL_BEAM_X               38      /* node 1:14                       */
#define CL_BEAM_Y               36
#define CL_ITALIC_X             9       /* node 1:6                        */
#define CL_ITALIC_Y             72

/**********************
 *     NAVIGATION
 **********************/
#define CL_DISC_X               306     /* node 1:27                       */
#define CL_DISC_Y               50
#define CL_DISC_D               55
#define CL_MANOEUVRE_X          312     /* node 1:28                       */
#define CL_MANOEUVRE_Y          56
#define CL_STREET_X             376     /* node 1:24                       */
#define CL_STREET_Y             54
#define CL_DIST_X               376     /* node 1:25                       */
#define CL_DIST_Y               80

/**********************
 *       SPEED
 **********************/
#define CL_TURN_L_X             246     /* node 1:12 (1:10 turned round)   */
#define CL_TURN_R_X             493     /* node 1:10                       */
#define CL_TURN_Y               124
#define CL_TURN_D               60
#define CL_SPEED_X              316     /* node 1:8                        */
#define CL_SPEED_Y              130
#define CL_UNIT_X               375     /* node 1:9                        */
#define CL_UNIT_Y               281

/**********************
 *     RIDE MODES
 **********************/
#define CL_MODE_X               83      /* nodes 1:30, 1:31, 1:32          */
#define CL_MODE_PREV_Y          165     /* CRAWL, above the pill           */
#define CL_MODE_NEXT_Y          294     /* RUSH, below it                  */
/* Node 1:32 reports 130x54 at (83,213) with a 2px stroke, but that stroke is
 * centre-aligned, so half of it falls outside the node - the painted box is
 * 132x56 at (82,212), radius 23.  LVGL draws borders inside an object, so
 * these are the painted numbers; the node's are recovered by insetting one
 * pixel.  It is the only place in the frame where the two differ. */
#define CL_PILL_X               82      /* node 1:32                       */
#define CL_PILL_Y               212
#define CL_PILL_W               132
#define CL_PILL_H               56
#define CL_PILL_R               23
#define CL_PILL_BORDER          2
#define CL_MODE_CURR_Y          226     /* node 1:33                       */
#define CL_MODE_DOT_X           53      /* node 1:22                       */
#define CL_MODE_DOT_Y           236
#define CL_DOT_D                12

/**********************
 *    RIGHT COLUMN
 **********************/
#define CL_ODO_CX               683     /* node 1:43, centred              */
#define CL_ODO_Y                179
#define CL_ODO_CAP_X            646     /* node 1:44                       */
#define CL_ODO_CAP_Y            207
#define CL_RANGE_X              617     /* node 1:45                       */
#define CL_RANGE_Y              241
#define CL_RANGE_W              137
#define CL_RANGE_H              60
#define CL_RANGE_R              15
#define CL_RANGE_CAP_X          658     /* node 1:46                       */
#define CL_RANGE_CAP_Y          249
#define CL_RANGE_VAL_X          649     /* node 1:47                       */
#define CL_RANGE_VAL_Y          268

/**********************
 *    AUTO BALANCE
 **********************/
/* Node 1:23 is a cubic, not a circular arc - a chord of 300 with a rise of
 * 25.5 in the middle.  Fitting a circle to it drifts about two pixels at the
 * quarter points, so the curve is flattened from its real control points and
 * drawn as a polyline. */
#define CL_CURVE_P0_X           250.0f
#define CL_CURVE_P0_Y           390.5f
#define CL_CURVE_P1_X           367.2f
#define CL_CURVE_P1_Y           356.3f
#define CL_CURVE_P2_X           433.0f
#define CL_CURVE_P2_Y           356.7f
#define CL_CURVE_P3_X           550.0f
#define CL_CURVE_P3_Y           390.5f
#define CL_CURVE_STEPS          32
#define CL_CURVE_DOT_X          327    /* node 1:21                        */
#define CL_CURVE_DOT_Y          364
#define CL_AB_CX                400    /* node 1:16, centred               */
#define CL_AB_Y                 392
#define CL_AB_HINT_CX           400    /* node 1:17, centred               */
#define CL_AB_HINT_Y            422

/**********************
 *     RIDE MODES
 **********************/
typedef enum {
    CL_MODE_CRAWL  = 0,
    CL_MODE_STREET = 1,
    CL_MODE_RUSH   = 2,
    CL_MODE_COUNT
} cl_mode_t;

#ifdef __cplusplus
}
#endif

#endif /* CL_THEME_H */
