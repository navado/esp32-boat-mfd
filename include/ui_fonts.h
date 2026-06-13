#pragma once

#include <lvgl.h>

// Custom large numeric font (Montserrat Medium, 64 px, 4 bpp) for hero
// readouts where the built-in montserrat_48 ceiling is too small. Generated
// from the LVGL bundled Montserrat-Medium.ttf with lv_font_conv; glyph set is
// digits + the symbols/letters used by instrument values
// ("0123456789.-°STPNEWkmndegVA% "). Source: src/fonts/font_xl_64.c.
LV_FONT_DECLARE(font_xl_64);
