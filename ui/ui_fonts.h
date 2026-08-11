/*
 * ui_fonts.h
 *
 * Typefaces for the cluster.  All generated with lv_font_conv from Outfit
 * (SIL Open Font License) - a geometric sans whose light weights give the
 * numerals the thin, calm look the design is after.
 *
 *   ev_font_speed_92   Outfit Thin        digits only, the hero readout
 *   ev_font_num_44     Outfit ExtraLight  digits, secondary values
 *   ev_font_num_28     Outfit Light       digits + unit letters
 *   ev_font_ui_16      Outfit Regular     body text (street names) + icons
 *   ev_font_cap_13     Outfit Medium      captions + icons
 *   ev_font_cap_11     Outfit Medium      micro captions
 *   ev_font_icon_26    FontAwesome 5      tell-tales
 *   ev_font_icon_38    FontAwesome 5      navigation manoeuvre arrow
 *
 * The big faces carry only the glyphs they need, which is why a 92 px font
 * costs about the same flash as a 16 px one.  See fonts/README.md to
 * regenerate at other sizes or with a different family.
 */

#ifndef UI_FONTS_H
#define UI_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

LV_FONT_DECLARE(ev_font_speed_92)
LV_FONT_DECLARE(ev_font_num_44)
LV_FONT_DECLARE(ev_font_num_28)
LV_FONT_DECLARE(ev_font_ui_16)
LV_FONT_DECLARE(ev_font_cap_13)
LV_FONT_DECLARE(ev_font_cap_11)
LV_FONT_DECLARE(ev_font_icon_26)
LV_FONT_DECLARE(ev_font_icon_38)

#define EV_FONT_SPEED   &ev_font_speed_92
#define EV_FONT_NUM_L   &ev_font_num_44
#define EV_FONT_NUM_M   &ev_font_num_28
#define EV_FONT_UI      &ev_font_ui_16
#define EV_FONT_CAP     &ev_font_cap_13
#define EV_FONT_CAP_S   &ev_font_cap_11
#define EV_FONT_ICON    &ev_font_icon_26
#define EV_FONT_ICON_L  &ev_font_icon_38

/* ---- FontAwesome 5 code points, UTF-8 encoded ------------------------ *
 * These replace LV_SYMBOL_* because the Montserrat faces that normally
 * carry those glyphs are not linked in.                                  */
#define EV_ICON_ARROW_L   "\xEF\x81\xA0"   /* F060  turn indicator, left   */
#define EV_ICON_ARROW_R   "\xEF\x81\xA1"   /* F061  turn indicator, right  */
#define EV_ICON_ARROW_U   "\xEF\x81\xA2"   /* F062  nav: continue straight */
#define EV_ICON_TURN_R    "\xEF\x81\xA4"   /* F064  nav: turn right        */
#define EV_ICON_TURN_L    "\xEF\x8F\xA5"   /* F3E5  nav: turn left         */
#define EV_ICON_UTURN     "\xEF\x83\xA2"   /* F0E2  nav: u-turn            */
#define EV_ICON_BOLT      "\xEF\x83\xA7"   /* F0E7  charging               */
#define EV_ICON_BT        "\xEF\x8A\x93"   /* F293  bluetooth              */
#define EV_ICON_GPS       "\xEF\x8F\x85"   /* F3C5  location fix           */
#define EV_ICON_LOCARROW  "\xEF\x84\xA4"   /* F124  location arrow         */
#define EV_ICON_SUN       "\xEF\x86\x85"   /* F185  weather                */
#define EV_ICON_EYE       "\xEF\x81\xAE"   /* F06E  head lamp              */
#define EV_ICON_WARN      "\xEF\x81\xB1"   /* F071  fault                  */
#define EV_ICON_BATT      "\xEF\x89\x80"   /* F240  battery                */

#ifdef __cplusplus
}
#endif

#endif /* UI_FONTS_H */
