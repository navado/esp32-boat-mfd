#include "screens.h"
#include "ui_screens.h"
#include "ui_theme.h"
#include "ui_compass.h"
#include "ui_data.h"
#include "ui_dirty.h"
#include "ui_fonts.h"
#include "signalk.h"
#include "board_pins.h"

#include <math.h>
#include <stdio.h>

// Wind-steering screen: a sailing aid built around the TRUE WIND ANGLE, with the
// tack / gybe course-change angles, the resulting opposite-tack/gybe heading,
// the true wind direction, and the apparent/true wind angles + speeds. No title
// chip (navigated by swipe). Tile-based so it adapts cleanly to 480/800/1024.
//
//   tack angle  = 2 * |TWA off bow|              (angle between the two beats)
//   gybe angle  = 2 * (180 - |TWA off bow|)      (angle between the two runs)
//   TWD (wind from, true) = heading + TWA(bow-relative)
//   opposite heading = mirror of heading across the wind axis = 2*TWD - heading
//     (this is the new heading after a tack when beating, or after a gybe when
//      running, holding the same wind angle)

namespace ui::wind_steer {

static lv_obj_t *s_root = nullptr;
static lv_obj_t *lbl_twa = nullptr;
static lv_obj_t *lbl_src = nullptr;  // "polars: SignalK" vs "polars: estimated"
static lv_obj_t *lbl_tack_val, *lbl_tack_hdg;
static lv_obj_t *lbl_gybe_val, *lbl_gybe_hdg;
static lv_obj_t *tile_awa, *tile_aws, *tile_tws, *tile_twd;

// A two-line steering panel: caption (top-left), big angle value (centre), and a
// small "-> ddd deg" heading line (bottom). Returns the value + heading labels.
static void steer_panel(lv_obj_t *parent, int x, int y, int w, int h, const char *cap,
                        uint32_t color, lv_obj_t **out_val, lv_obj_t **out_hdg) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    style_panel(box);
    lv_obj_set_style_pad_all(box, 8, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *c = lv_label_create(box);
    lv_label_set_text(c, cap);
    lv_obj_set_style_text_font(c, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(c, lv_color_hex(theme.fg_dim), 0);
    lv_obj_align(c, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *v = lv_label_create(box);
    lv_label_set_text(v, "--\xC2\xB0");
    lv_obj_set_style_text_font(v, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(v, lv_color_hex(color), 0);
    lv_obj_align(v, LV_ALIGN_CENTER, 0, 2);
    *out_val = v;

    lv_obj_t *hd = lv_label_create(box);
    lv_label_set_text(hd, LV_SYMBOL_RIGHT " ---\xC2\xB0");
    lv_obj_set_style_text_font(hd, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hd, lv_color_hex(theme.fg), 0);
    lv_obj_align(hd, LV_ALIGN_BOTTOM_MID, 0, 0);
    *out_hdg = hd;
}

lv_obj_t *build(lv_obj_t *parent) {
    s_root = lv_obj_create(parent);
    lv_obj_set_size(s_root, LCD_W, LCD_H);
    lv_obj_set_pos(s_root, 0, 0);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(theme.bg), 0);
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_radius(s_root, 0, 0);
    lv_obj_set_style_pad_all(s_root, 0, 0);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_root, LV_OBJ_FLAG_EVENT_BUBBLE);

    bool wide = (LCD_W * 100 > LCD_H * 125);
    int gap = 8;

    // --- hero: TRUE WIND ANGLE ---
    lv_obj_t *cap = lv_label_create(s_root);
    lv_label_set_text(cap, "TRUE WIND ANGLE");
    lv_obj_set_style_text_font(cap, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(cap, lv_color_hex(theme.fg_dim), 0);
    lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 10);

    lbl_twa = lv_label_create(s_root);
    lv_label_set_text(lbl_twa, "--\xC2\xB0");
    lv_obj_set_style_text_font(lbl_twa, &font_xl_64, 0);
    lv_obj_set_style_text_color(lbl_twa, lv_color_hex(theme.accent), 0);
    lv_obj_set_style_text_align(lbl_twa, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_twa, LV_ALIGN_TOP_MID, 0, 38);

    // Polar-source indicator (top-right): SignalK polars vs empirical estimate.
    lbl_src = lv_label_create(s_root);
    lv_label_set_text(lbl_src, "");
    lv_obj_set_style_text_font(lbl_src, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_src, lv_color_hex(theme.fg_dim), 0);
    lv_obj_align(lbl_src, LV_ALIGN_TOP_RIGHT, -10, 10);

    int hero_h = wide ? 150 : 132;

    // --- tack / gybe panels + wind tiles ---
    // Square: TACK | GYBE on one row, then AWA AWS TWS TWD across the bottom.
    // Wide: all six in a single row.
    const lv_font_t *tv = &lv_font_montserrat_38;
    if (!wide) {
        int pw = (LCD_W - 3 * gap) / 2;
        int ph = 120;
        int py = hero_h + gap;
        steer_panel(s_root, gap, py, pw, ph, "TACK", theme.starboard, &lbl_tack_val, &lbl_tack_hdg);
        steer_panel(s_root, gap * 2 + pw, py, pw, ph, "GYBE", theme.warn, &lbl_gybe_val,
                    &lbl_gybe_hdg);

        int n = 4;
        int tw = (LCD_W - (n + 1) * gap) / n;
        int ty = py + ph + gap;
        int th = LCD_H - ty - gap;
        tile_awa = ui::numeric_tile(s_root, gap, ty, tw, th, "AWA", "", tv, theme.fg);
        tile_aws = ui::numeric_tile(s_root, gap * 2 + tw, ty, tw, th, "AWS", "kn", tv, theme.warn);
        tile_tws =
            ui::numeric_tile(s_root, gap * 3 + tw * 2, ty, tw, th, "TWS", "kn", tv, theme.fg);
        tile_twd =
            ui::numeric_tile(s_root, gap * 4 + tw * 3, ty, tw, th, "TWD", "", tv, theme.accent);
    } else {
        int n = 6;
        int tw = (LCD_W - (n + 1) * gap) / n;
        int ty = hero_h + gap;
        int th = LCD_H - ty - gap;
        steer_panel(s_root, gap, ty, tw, th, "TACK", theme.starboard, &lbl_tack_val, &lbl_tack_hdg);
        steer_panel(s_root, gap * 2 + tw, ty, tw, th, "GYBE", theme.warn, &lbl_gybe_val,
                    &lbl_gybe_hdg);
        tile_awa = ui::numeric_tile(s_root, gap * 3 + tw * 2, ty, tw, th, "AWA", "", tv, theme.fg);
        tile_aws =
            ui::numeric_tile(s_root, gap * 4 + tw * 3, ty, tw, th, "AWS", "kn", tv, theme.warn);
        tile_tws =
            ui::numeric_tile(s_root, gap * 5 + tw * 4, ty, tw, th, "TWS", "kn", tv, theme.fg);
        tile_twd =
            ui::numeric_tile(s_root, gap * 6 + tw * 5, ty, tw, th, "TWD", "", tv, theme.accent);
    }

    return s_root;
}

// ---- refresh ----

static char s_last_twa[12] = {(char)0xFF};
static char s_last_tackv[8] = {(char)0xFF};
static char s_last_tackh[12] = {(char)0xFF};
static char s_last_gybev[8] = {(char)0xFF};
static char s_last_gybeh[12] = {(char)0xFF};
static char s_last_awa[12] = {(char)0xFF};
static char s_last_aws[12] = {(char)0xFF};
static char s_last_tws[12] = {(char)0xFF};
static char s_last_twd[12] = {(char)0xFF};
static char s_last_src[20] = {(char)0xFF};

static double norm360(double d) {
    while (d < 0)
        d += 360;
    while (d >= 360)
        d -= 360;
    return d;
}

void refresh() {
    sk::Data d;
    sk::copyData(d);
    char buf[24];

    // Geometry: TWA off bow (+stbd), current tack side, TWD (wind FROM), and the
    // optimal beat / gybe angles from SignalK polars when available.
    double twa_raw = isnan(d.twa) ? NAN : rad_to_deg_pos(d.twa);  // 0..360
    bool stbd = !isnan(twa_raw) && twa_raw <= 180.0;              // wind on stbd => stbd tack
    double twa_mag = isnan(twa_raw) ? NAN : (stbd ? twa_raw : 360.0 - twa_raw);
    bool beating = !isnan(twa_mag) && twa_mag <= 90.0;
    double hdg = isnan(d.headingTrue) ? NAN : rad_to_deg_pos(d.headingTrue);
    double twd = (!isnan(hdg) && !isnan(twa_raw)) ? norm360(hdg + twa_raw) : NAN;
    double beat =
        isnan(d.beatAngle) ? NAN : rad_to_deg_pos(d.beatAngle);  // polar optimal upwind TWA
    double gybe =
        isnan(d.gybeAngle) ? NAN : rad_to_deg_pos(d.gybeAngle);  // polar optimal downwind TWA
    bool have_polar = !isnan(beat) || !isnan(gybe);

    // --- TWA hero ---
    if (!isnan(twa_mag)) {
        snprintf(buf, sizeof(buf), "%.0f\xC2\xB0%c", twa_mag, stbd ? 'S' : 'P');
        set_text_if_changed(lbl_twa, s_last_twa, sizeof(s_last_twa), buf);
    } else {
        set_text_if_changed(lbl_twa, s_last_twa, sizeof(s_last_twa), "--\xC2\xB0");
    }

    // --- TACK: tacking angle + the optimal close-hauled heading on the other
    // tack. Polar (2*beatAngle, laylines TWD +/- beat) when available, else the
    // empirical current-TWA mirror (only meaningful while beating). ---
    char tv[8], th[16];
    if (!isnan(beat)) {
        snprintf(tv, sizeof(tv), "%.0f\xC2\xB0", 2.0 * beat);
        if (!isnan(twd))
            snprintf(th, sizeof(th), LV_SYMBOL_RIGHT " %03.0f\xC2\xB0",
                     norm360(twd + (stbd ? beat : -beat)));
        else
            snprintf(th, sizeof(th), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    } else if (beating) {
        snprintf(tv, sizeof(tv), "%.0f\xC2\xB0", 2.0 * twa_mag);
        if (!isnan(twd) && !isnan(hdg))
            snprintf(th, sizeof(th), LV_SYMBOL_RIGHT " %03.0f\xC2\xB0", norm360(2.0 * twd - hdg));
        else
            snprintf(th, sizeof(th), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    } else {
        snprintf(tv, sizeof(tv), "--\xC2\xB0");
        snprintf(th, sizeof(th), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    }
    set_text_if_changed(lbl_tack_val, s_last_tackv, sizeof(s_last_tackv), tv);
    set_text_if_changed(lbl_tack_hdg, s_last_tackh, sizeof(s_last_tackh), th);

    // --- GYBE: gybing angle + the optimal running heading on the other gybe. ---
    char gv[8], gh[16];
    if (!isnan(gybe)) {
        snprintf(gv, sizeof(gv), "%.0f\xC2\xB0", 2.0 * (180.0 - gybe));
        if (!isnan(twd))
            snprintf(gh, sizeof(gh), LV_SYMBOL_RIGHT " %03.0f\xC2\xB0",
                     norm360(twd + (stbd ? gybe : -gybe)));
        else
            snprintf(gh, sizeof(gh), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    } else if (!beating && !isnan(twa_mag)) {
        snprintf(gv, sizeof(gv), "%.0f\xC2\xB0", 2.0 * (180.0 - twa_mag));
        if (!isnan(twd) && !isnan(hdg))
            snprintf(gh, sizeof(gh), LV_SYMBOL_RIGHT " %03.0f\xC2\xB0", norm360(2.0 * twd - hdg));
        else
            snprintf(gh, sizeof(gh), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    } else {
        snprintf(gv, sizeof(gv), "--\xC2\xB0");
        snprintf(gh, sizeof(gh), LV_SYMBOL_RIGHT " ---\xC2\xB0");
    }
    set_text_if_changed(lbl_gybe_val, s_last_gybev, sizeof(s_last_gybev), gv);
    set_text_if_changed(lbl_gybe_hdg, s_last_gybeh, sizeof(s_last_gybeh), gh);

    // --- TWD tile + polar-source indicator ---
    if (!isnan(twd)) {
        snprintf(buf, sizeof(buf), "%03.0f\xC2\xB0", twd);
        set_text_if_changed(tile_twd, s_last_twd, sizeof(s_last_twd), buf);
    } else {
        set_text_if_changed(tile_twd, s_last_twd, sizeof(s_last_twd), "--\xC2\xB0");
    }
    set_text_if_changed(lbl_src, s_last_src, sizeof(s_last_src),
                        have_polar ? "polars: SignalK" : "polars: estimated");

    // AWA (off bow, P/S).
    if (!isnan(d.awa)) {
        double raw = rad_to_deg_pos(d.awa);
        bool stbd = raw <= 180.0;
        snprintf(buf, sizeof(buf), "%.0f%c", stbd ? raw : 360.0 - raw, stbd ? 'S' : 'P');
        set_text_if_changed(tile_awa, s_last_awa, sizeof(s_last_awa), buf);
    } else {
        set_text_if_changed(tile_awa, s_last_awa, sizeof(s_last_awa), "--");
    }
    if (!isnan(d.aws)) {
        snprintf(buf, sizeof(buf), "%.1f", mps_to_kn(d.aws));
        set_text_if_changed(tile_aws, s_last_aws, sizeof(s_last_aws), buf);
    } else {
        set_text_if_changed(tile_aws, s_last_aws, sizeof(s_last_aws), "--");
    }
    if (!isnan(d.tws)) {
        snprintf(buf, sizeof(buf), "%.1f", mps_to_kn(d.tws));
        set_text_if_changed(tile_tws, s_last_tws, sizeof(s_last_tws), buf);
    } else {
        set_text_if_changed(tile_tws, s_last_tws, sizeof(s_last_tws), "--");
    }
}

}  // namespace ui::wind_steer
