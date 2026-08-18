/*
 * cl_icons.h
 *
 * Every glyph in the frame that is not type.  These are ALPHA_8BIT maps -
 * coverage only, no colour - so the same bitmap can be tinted per state by
 * setting img_recolor on the widget.  Generated from the SVGs Figma
 * exported; see icons/generate.py.
 */

#ifndef CL_ICONS_H
#define CL_ICONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

LV_IMAGE_DECLARE(cl_icon_italic_i);    /* node 1:6                            */
LV_IMAGE_DECLARE(cl_icon_headlight);   /* node 1:14, low beam                 */
LV_IMAGE_DECLARE(cl_icon_warning);     /* node 1:19                           */
LV_IMAGE_DECLARE(cl_icon_bluetooth);   /* node 1:37                           */
LV_IMAGE_DECLARE(cl_icon_navigation);  /* node 1:39                           */
LV_IMAGE_DECLARE(cl_icon_battery);     /* node 1:48                           */
LV_IMAGE_DECLARE(cl_icon_turn_left);   /* node 1:12                           */
LV_IMAGE_DECLARE(cl_icon_turn_right);  /* node 1:10                           */
LV_IMAGE_DECLARE(cl_icon_manoeuvre);   /* node 1:28, inside the nav disc      */

LV_IMAGE_DECLARE(cl_glow_pill);        /* node 1:32's inset glow, baked       */
LV_IMAGE_DECLARE(cl_shade_panel);      /* node 1:45's inset shade, baked      */
LV_IMAGE_DECLARE(cl_pool_src);         /* node 1:3's gradient at 1/4 scale    */

#ifdef __cplusplus
}
#endif

#endif /* CL_ICONS_H */
