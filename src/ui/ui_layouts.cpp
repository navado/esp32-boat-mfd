#include "ui_layouts.h"

#include "ui_theme.h"
#include "ui_data.h"
#include "ui_dirty.h"
#include "ui_screens.h"
#include "board_pins.h"
#include "app_events.h"
#include "net.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace ui::layouts {

// ---------------------------------------------------------------------------
// Per-template instance state. We attach this as user_data on the
// template's root object so update() can find its labels/widgets
// without depending on file-scope statics (templates can be
// instantiated multiple times, e.g. test screens).

struct QuadGridTile {
    lv_obj_t *root;
    lv_obj_t *cap;
    lv_obj_t *value;
    lv_obj_t *unit;
    lv_obj_t *secondary;
    lv_obj_t *extras[4];      // multi-row tiles - small label+value lines
    char last_value[24];
    char last_secondary[24];
    char last_extras[4][32];
    int idx;  // metric index into spec.metrics[]
};

struct QuadGridState {
    QuadGridTile tiles[4];
};

// ---------------------------------------------------------------------------
// Metric formatting. Returns the primary value text, and optionally
// fills `secondary` with a short context string. NaN -> "--".

static void format_metric(const MetricBinding &m, const sk::Data &d,
                          char *primary, size_t pcap,
                          char *secondary, size_t scap) {
    secondary[0] = 0;
    switch (m.source) {
    case MetricSource::AWS_kn:
        if (!isnan(d.aws)) snprintf(primary, pcap, "%.1f", mps_to_kn(d.aws));
        else snprintf(primary, pcap, "--");
        if (!isnan(d.awa)) {
            double deg = rad_to_deg_pos(d.awa);
            bool stbd = deg <= 180.0;
            snprintf(secondary, scap, "%.0f%c", stbd ? deg : 360 - deg,
                     stbd ? 'S' : 'P');
        }
        break;
    case MetricSource::TWS_kn:
        if (!isnan(d.tws)) snprintf(primary, pcap, "%.1f", mps_to_kn(d.tws));
        else snprintf(primary, pcap, "--");
        if (!isnan(d.twa)) {
            double deg = rad_to_deg_pos(d.twa);
            bool stbd = deg <= 180.0;
            snprintf(secondary, scap, "%.0f%c", stbd ? deg : 360 - deg,
                     stbd ? 'S' : 'P');
        }
        break;
    case MetricSource::AWA_deg:
        if (!isnan(d.awa)) {
            double deg = rad_to_deg_pos(d.awa);
            bool stbd = deg <= 180.0;
            snprintf(primary, pcap, "%.0f%c", stbd ? deg : 360 - deg,
                     stbd ? 'S' : 'P');
        } else {
            snprintf(primary, pcap, "--");
        }
        break;
    case MetricSource::TWA_deg:
        if (!isnan(d.twa)) {
            double deg = rad_to_deg_pos(d.twa);
            bool stbd = deg <= 180.0;
            snprintf(primary, pcap, "%.0f%c", stbd ? deg : 360 - deg,
                     stbd ? 'S' : 'P');
        } else {
            snprintf(primary, pcap, "--");
        }
        break;
    case MetricSource::SOG_kn:
        if (!isnan(d.sog)) snprintf(primary, pcap, "%.1f", mps_to_kn(d.sog));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::COG_deg:
        if (!isnan(d.cogTrue)) snprintf(primary, pcap, "%03.0f", rad_to_deg_pos(d.cogTrue));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::HDG_deg:
        if (!isnan(d.headingTrue)) snprintf(primary, pcap, "%03.0f", rad_to_deg_pos(d.headingTrue));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::Depth_m:
        if (!isnan(d.depth)) snprintf(primary, pcap, "%.1f", d.depth);
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::WaterTemp_C:
        if (!isnan(d.waterTemp)) snprintf(primary, pcap, "%.1f", k_to_c(d.waterTemp));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::BatteryV:
        if (!isnan(d.battVoltage)) snprintf(primary, pcap, "%.2f", d.battVoltage);
        else snprintf(primary, pcap, "--");
        if (!isnan(d.battSoc)) {
            snprintf(secondary, scap, "%.0f%%", d.battSoc * 100.0);
        }
        break;
    case MetricSource::BatterySOC_pct:
        if (!isnan(d.battSoc)) snprintf(primary, pcap, "%.0f%%", d.battSoc * 100.0);
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::DTW:
        if (!isnan(d.dtw)) {
            if (d.dtw >= 1852.0) snprintf(primary, pcap, "%.2f", d.dtw / 1852.0);
            else snprintf(primary, pcap, "%.0f", d.dtw);
        } else {
            snprintf(primary, pcap, "--");
        }
        break;
    case MetricSource::BTW_deg:
        if (!isnan(d.btw)) snprintf(primary, pcap, "%03.0f", rad_to_deg_pos(d.btw));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::XTE:
        if (!isnan(d.xte)) snprintf(primary, pcap, "%.0f", fabs(d.xte));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::VMG_kn:
        if (!isnan(d.vmg)) snprintf(primary, pcap, "%.1f", mps_to_kn(d.vmg));
        else snprintf(primary, pcap, "--");
        break;
    case MetricSource::Position:
        if (!isnan(d.lat) && !isnan(d.lon)) {
            format_position(d.lat, d.lon, pos_format(), primary, pcap);
        } else {
            snprintf(primary, pcap, "no fix");
        }
        break;
    case MetricSource::APState:
        if (d.apState[0]) snprintf(primary, pcap, "%s", d.apState);
        else snprintf(primary, pcap, "off");
        break;
    case MetricSource::None:
    default:
        snprintf(primary, pcap, "--");
        break;
    }
}

// ---------------------------------------------------------------------------
// quad_grid template - 2x2 tile dashboard
//
// Layout: 4 tiles laid out in a 2x2 grid, each ~240x230 px on a 480x480
// panel. Top-of-screen has the MOB-safe band reserved (see spec 09
// safe zone); tiles start at y=72 and the bottom row reaches y=472.

static constexpr int QG_TOP_Y = 72;            // clear MOB pill
static constexpr int QG_BOTTOM_Y = LCD_H - 8;  // small bottom margin
static constexpr int QG_TILE_W = (LCD_W - 12) / 2;
static constexpr int QG_TILE_H = (QG_BOTTOM_Y - QG_TOP_Y - 4) / 2;
static constexpr int QG_GAP = 4;

static void tile_clicked_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    auto *target = (const char *)lv_event_get_user_data(e);
    net::logf("[layout] tile CLICKED target=%s", target ? target : "(null)");
    if (!target || !*target) return;
    app::Command c;
    c.type = app::CommandType::ShowScreen;
    strncpy(c.a, target, sizeof(c.a) - 1);
    c.t_post_us = micros();
    app::post(c, 0);
}

static QuadGridTile build_tile(lv_obj_t *parent, int x, int y, int w, int h,
                                const MetricBinding &m) {
    QuadGridTile t = {};
    t.idx = -1;
    strncpy(t.last_value, "\xFF", sizeof(t.last_value));
    strncpy(t.last_secondary, "\xFF", sizeof(t.last_secondary));
    for (int i = 0; i < 4; ++i) strncpy(t.last_extras[i], "\xFF", sizeof(t.last_extras[0]));

    t.root = lv_obj_create(parent);
    lv_obj_set_size(t.root, w, h);
    lv_obj_set_pos(t.root, x, y);
    lv_obj_set_style_bg_color(t.root, lv_color_hex(theme.panel), 0);
    lv_obj_set_style_bg_opa(t.root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(t.root, lv_color_hex(theme.panel_edge), 0);
    lv_obj_set_style_border_width(t.root, 1, 0);
    lv_obj_set_style_radius(t.root, 8, 0);
    lv_obj_set_style_pad_all(t.root, 10, 0);
    lv_obj_clear_flag(t.root, LV_OBJ_FLAG_SCROLLABLE);

    // Accent rail.
    lv_obj_t *rail = lv_obj_create(t.root);
    lv_obj_set_size(rail, 4, h - 20);
    lv_obj_set_pos(rail, 0, 0);
    lv_obj_set_style_bg_color(rail, lv_color_hex(m.accent), 0);
    lv_obj_set_style_bg_opa(rail, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rail, 0, 0);
    lv_obj_set_style_radius(rail, 2, 0);
    lv_obj_set_style_pad_all(rail, 0, 0);
    lv_obj_clear_flag(rail, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(rail, LV_OBJ_FLAG_CLICKABLE);

    // Caption.
    t.cap = lv_label_create(t.root);
    lv_label_set_text(t.cap, m.label ? m.label : "");
    lv_obj_set_style_text_font(t.cap, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(t.cap, lv_color_hex(theme.fg_dim), 0);
    lv_obj_set_pos(t.cap, 12, 4);
    lv_obj_clear_flag(t.cap, LV_OBJ_FLAG_CLICKABLE);

    bool has_extras = (m.extras_count > 0);

    // Primary value. If extras are present, shrink + pin to upper area
    // so the extras have room below. Otherwise it stays large + centered.
    t.value = lv_label_create(t.root);
    lv_label_set_text(t.value, "--");
    const lv_font_t *primary_font = has_extras ? &lv_font_montserrat_28
                                                : &lv_font_montserrat_48;
    lv_obj_set_style_text_font(t.value, primary_font, 0);
    lv_obj_set_style_text_color(t.value, lv_color_hex(theme.fg), 0);
    if (has_extras) {
        lv_obj_align(t.value, LV_ALIGN_TOP_LEFT, 12, 26);
    } else {
        lv_obj_align(t.value, LV_ALIGN_CENTER, 0, 0);
    }
    lv_obj_clear_flag(t.value, LV_OBJ_FLAG_CLICKABLE);

    // Unit.
    if (m.unit && m.unit[0]) {
        t.unit = lv_label_create(t.root);
        lv_label_set_text(t.unit, m.unit);
        lv_obj_set_style_text_font(t.unit, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(t.unit, lv_color_hex(theme.fg_dim), 0);
        if (has_extras) {
            lv_obj_align(t.unit, LV_ALIGN_TOP_LEFT, 12 + 90, 32);
        } else {
            lv_obj_align(t.unit, LV_ALIGN_CENTER, 0, 30);
        }
        lv_obj_clear_flag(t.unit, LV_OBJ_FLAG_CLICKABLE);
    }

    if (!has_extras) {
        // Classic Hero layout: secondary in bottom-right.
        t.secondary = lv_label_create(t.root);
        lv_label_set_text(t.secondary, "");
        lv_obj_set_style_text_font(t.secondary, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(t.secondary, lv_color_hex(theme.fg_dim), 0);
        lv_obj_align(t.secondary, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
        lv_obj_clear_flag(t.secondary, LV_OBJ_FLAG_CLICKABLE);
    } else {
        // Multi-row layout: stack up to 4 extras below the primary value.
        for (uint8_t i = 0; i < m.extras_count && i < 4; ++i) {
            t.extras[i] = lv_label_create(t.root);
            lv_label_set_text(t.extras[i], "");
            lv_obj_set_style_text_font(t.extras[i], &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(t.extras[i], lv_color_hex(theme.fg), 0);
            // Row positions packed below the primary value area.
            lv_obj_align(t.extras[i], LV_ALIGN_TOP_LEFT, 12, 76 + i * 22);
            lv_obj_clear_flag(t.extras[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }

    if (m.target_screen && m.target_screen[0]) {
        lv_obj_add_event_cb(t.root, tile_clicked_cb, LV_EVENT_CLICKED,
                            (void *)m.target_screen);
    } else {
        lv_obj_clear_flag(t.root, LV_OBJ_FLAG_CLICKABLE);
    }
    return t;
}

static lv_obj_t *create_quad_grid(lv_obj_t *parent, const ScreenVariantSpec &spec) {
    if (spec.metric_count < 1) return nullptr;
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LCD_W, LCD_H);
    if (parent) lv_obj_set_pos(root, 0, 0);
    lv_obj_set_style_bg_color(root, lv_color_hex(theme.bg), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_radius(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    QuadGridState *st = (QuadGridState *)heap_caps_calloc(1, sizeof(QuadGridState),
                                                          MALLOC_CAP_INTERNAL);
    if (!st) {
        net::logf("[layout] quad_grid alloc failed");
        return root;  // empty screen, caller still gets a valid handle
    }

    // 2x2 layout; missing metrics leave the tile slot blank.
    static const MetricBinding empty_metric = {"", "", "", MetricSource::None, 0x222222, nullptr};
    for (int i = 0; i < 4; ++i) {
        const MetricBinding &m = (i < spec.metric_count) ? spec.metrics[i] : empty_metric;
        int col = i % 2;
        int row = i / 2;
        int x = QG_GAP + col * (QG_TILE_W + QG_GAP);
        int y = QG_TOP_Y + row * (QG_TILE_H + QG_GAP);
        st->tiles[i] = build_tile(root, x, y, QG_TILE_W, QG_TILE_H, m);
        st->tiles[i].idx = (i < spec.metric_count) ? i : -1;
    }

    lv_obj_set_user_data(root, st);
    return root;
}

static void update_quad_grid(lv_obj_t *root, const ScreenVariantSpec &spec,
                              const sk::Data &data) {
    if (!root) return;
    auto *st = (QuadGridState *)lv_obj_get_user_data(root);
    if (!st) return;
    for (int i = 0; i < 4; ++i) {
        QuadGridTile &t = st->tiles[i];
        if (t.idx < 0 || t.idx >= spec.metric_count) continue;
        const MetricBinding &m = spec.metrics[t.idx];

        char pri[24], sec[24];
        format_metric(m, data, pri, sizeof(pri), sec, sizeof(sec));
        ui::set_text_if_changed(t.value, t.last_value, sizeof(t.last_value), pri);

        if (t.secondary) {
            ui::set_text_if_changed(t.secondary, t.last_secondary,
                                    sizeof(t.last_secondary), sec);
        }
        // Render extras (multi-row tiles). Each extra reuses
        // format_metric on a synthetic MetricBinding so all the
        // unit/format logic is shared.
        for (uint8_t e = 0; e < m.extras_count && e < 4; ++e) {
            if (!t.extras[e]) continue;
            MetricBinding eb = {};
            eb.source = m.extras[e].source;
            char ep[24], esec[24];
            format_metric(eb, data, ep, sizeof(ep), esec, sizeof(esec));
            char row[32];
            if (m.extras[e].label && m.extras[e].label[0]) {
                snprintf(row, sizeof(row), "%s %s", m.extras[e].label, ep);
            } else {
                snprintf(row, sizeof(row), "%s", ep);
            }
            ui::set_text_if_changed(t.extras[e], t.last_extras[e],
                                    sizeof(t.last_extras[e]), row);
        }
    }
}

// ---------------------------------------------------------------------------
// Public factory entry points

lv_obj_t *create(lv_obj_t *parent, const ScreenVariantSpec &spec) {
    switch (spec.template_id) {
    case TemplateId::QuadGrid: return create_quad_grid(parent, spec);
    default:
        net::logf("[layout] template %d not implemented yet", (int)spec.template_id);
        return nullptr;
    }
}

void update(lv_obj_t *root, const ScreenVariantSpec &spec, const sk::Data &data) {
    switch (spec.template_id) {
    case TemplateId::QuadGrid: update_quad_grid(root, spec, data); break;
    default: break;
    }
}

}  // namespace ui::layouts
