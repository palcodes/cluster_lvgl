/*
 * cl_fonts.h
 *
 * The ten faces the Figma frame calls for.  Two families only:
 *
 *   Bai Jamjuree Medium   the speed numeral (136 px) and the RANGE caption
 *   Jura SemiBold / Bold  everything else
 *
 * Both are SIL OFL 1.1.  Sizes are the design's pixel sizes, not point
 * sizes.  Regenerate with fonts/generate.sh, which also documents which
 * glyphs each face carries - several are subsetted to just their caption.
 */

#ifndef CL_FONTS_H
#define CL_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

LV_FONT_DECLARE(cl_font_speed_136)  /* Bai Jamjuree Medium, digits only     */
LV_FONT_DECLARE(cl_font_range_15)   /* Bai Jamjuree Medium, caps            */

LV_FONT_DECLARE(cl_font_bold_22)    /* Jura Bold - AUTO BALANCE             */
LV_FONT_DECLARE(cl_font_bold_16)    /* Jura Bold - KM/HR                    */
LV_FONT_DECLARE(cl_font_bold_14)    /* Jura Bold - the AB hint line         */

LV_FONT_DECLARE(cl_font_semi_24)    /* Jura SemiBold - mode, odo, range     */
LV_FONT_DECLARE(cl_font_semi_20)    /* Jura SemiBold - street, CRAWL/RUSH   */
LV_FONT_DECLARE(cl_font_semi_18)    /* Jura SemiBold - distance, network    */
LV_FONT_DECLARE(cl_font_semi_16)    /* Jura SemiBold - TOTAL KM             */
LV_FONT_DECLARE(cl_font_semi_14)    /* Jura SemiBold - clock, state of charge */

/*
 * A face plus the gap between where Figma puts a text node and where LVGL
 * puts a label.
 *
 * Both position text by the top of its line box, but they disagree about
 * how tall that box is: Figma's "leading: normal" comes from the browser's
 * rules, while LVGL uses the line_height lv_font_conv reads out of the
 * font's own metrics.  The difference is a constant per face - small for
 * the captions, forty pixels for the 136px numeral - so it is measured once
 * and carried here rather than being baked into the layout constants, which
 * stay equal to the node coordinates they came from.
 *
 * The numbers come from test/compare_to_figma.py.  Regenerate a face at a
 * different size and its lead has to be re-measured with it.
 */
typedef struct {
    const lv_font_t *font;
    int32_t       lead;
} cl_face_t;

extern const cl_face_t CL_SPEED_136;
extern const cl_face_t CL_RANGE_15;
extern const cl_face_t CL_BOLD_22;
extern const cl_face_t CL_BOLD_16;
extern const cl_face_t CL_BOLD_14;
extern const cl_face_t CL_SEMI_24;
extern const cl_face_t CL_SEMI_20;
extern const cl_face_t CL_SEMI_18;
extern const cl_face_t CL_SEMI_16;
extern const cl_face_t CL_SEMI_14;

#ifdef __cplusplus
}
#endif

#endif /* CL_FONTS_H */
