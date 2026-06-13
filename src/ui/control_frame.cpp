// Global "controlled" frame overlay (Phase 3.2). While the device is under
// remote control, each active controller session draws one thin nested border
// on lv_layer_top(), in that controller's color, inset by frame_step * i
// (outermost = most-recent). A small name-pill (top-center) shows the
// most-recent controller's name. No active sessions => everything hidden.
//
// LVGL discipline: build() and set_sessions() run ONLY on the UI/LVGL task.
// build() is called once at boot; set_sessions() is called from ui_refresh
// with the snapshot from proto_target::active_session_snapshot(). It
// dirty-compares (count + per-ring color) so it only repaints on change.
//
// Style values (border width, inset step, radius, pill metrics/colors) come
// from ui::chrome::control_frame in include/ui_theme.h — no inline magic
// numbers beyond loop indices.

#include <lvgl.h>

#include <cstdlib>
#include <cstring>

#include "board.h"
#include "proto/proto.h"
#include "screens.h"
#include "ui_theme.h"

namespace ui {
namespace control_frame {

namespace {

namespace cf = ui::chrome::control_frame;

// One lv_obj per possible session (transparent fill + colored border ring),
// plus a single name-pill (label) for the most-recent controller. All live on
// lv_layer_top() and start hidden.
lv_obj_t *s_rings[proto::kMaxSessions] = {nullptr};
lv_obj_t *s_pill = nullptr;
lv_obj_t *s_pill_lbl = nullptr;

// Dirty-compare cache so set_sessions() only mutates LVGL state on change.
int s_last_count = -1;
uint32_t s_last_colors[proto::kMaxSessions] = {0};
char s_last_name[48] = {0};

// Parse a "#RRGGBB" controller color. Falls back to the theme accent when the
// string is empty or malformed (defensive; controllers always send a color).
uint32_t parse_color(const char *c) {
    if (c && c[0] == '#') {
        char *end = nullptr;
        unsigned long v = strtoul(c + 1, &end, 16);
        if (end && *end == 0) return (uint32_t)(v & 0xffffff);
    }
    return ui::theme.accent;
}

}  // namespace

lv_obj_t *build(lv_obj_t * /*parent*/) {
    lv_obj_t *top = lv_layer_top();

    board::Geometry g = board::geometry();
    const bool round = (g.shape == board::DisplayShape::Round);
    // On a round panel the border follows the inscribed circle (radius huge so
    // LVGL clamps to a full circle); on rectangular panels it uses the panel
    // corner radius. Round rings are sized to the usable inscribed square so
    // they sit clear of the bezel; rectangular rings span the full panel.
    const int base_x = round ? g.usable_x : 0;
    const int base_y = round ? g.usable_y : 0;
    const int base_w = round ? g.usable_width : g.width_px;
    const int base_h = round ? g.usable_height : g.height_px;
    const int ring_radius = round ? LV_RADIUS_CIRCLE : cf::rect_radius;

    for (int i = 0; i < proto::kMaxSessions; ++i) {
        lv_obj_t *r = lv_obj_create(top);
        const int inset = i * cf::frame_step;
        lv_obj_set_size(r, base_w - 2 * inset, base_h - 2 * inset);
        lv_obj_set_pos(r, base_x + inset, base_y + inset);
        lv_obj_set_style_radius(r, ring_radius, 0);
        // Transparent fill: only the colored border shows.
        lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(r, cf::border_width, 0);
        lv_obj_set_style_border_opa(r, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(r, 0, 0);
        lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
        s_rings[i] = r;
    }

    // Name-pill: top-center, shows the most-recent controller's name on a
    // background of that controller's color.
    s_pill = lv_obj_create(top);
    lv_obj_set_style_radius(s_pill, cf::pill_radius, 0);
    lv_obj_set_style_border_width(s_pill, 0, 0);
    lv_obj_set_style_bg_opa(s_pill, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(s_pill, cf::pill_pad_x, 0);
    lv_obj_set_style_pad_ver(s_pill, cf::pill_pad_y, 0);
    lv_obj_set_size(s_pill, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(s_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_pill, LV_OBJ_FLAG_CLICKABLE);
    // On a round panel anchor the pill to the top of the usable circle.
    lv_obj_align(s_pill, LV_ALIGN_TOP_MID, 0, base_y + cf::pill_margin_top);
    lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);

    s_pill_lbl = lv_label_create(s_pill);
    lv_obj_set_style_text_color(s_pill_lbl, lv_color_hex(cf::pill_text), 0);
    lv_label_set_text(s_pill_lbl, "");
    lv_obj_center(s_pill_lbl);

    return s_pill;
}

void set_sessions(const proto::Session *s, int count) {
    if (!s_pill) return;  // build() not run yet (non-UI build path)
    if (count < 0) count = 0;
    if (count > proto::kMaxSessions) count = proto::kMaxSessions;

    // Order most-recent first (outermost ring + the pill) by lastSeen desc.
    // The snapshot arrives in slot-index order; reorder here so the visual
    // "most-recent outermost" contract holds regardless of slot layout. We
    // sort indices into the caller's array (small N, simple insertion sort).
    int order[proto::kMaxSessions];
    for (int i = 0; i < count; ++i)
        order[i] = i;
    for (int i = 1; i < count; ++i) {
        int j = i;
        while (j > 0 && s[order[j]].lastSeen > s[order[j - 1]].lastSeen) {
            int t = order[j];
            order[j] = order[j - 1];
            order[j - 1] = t;
            --j;
        }
    }

    // Dirty-compare: count, per-ring color, and the most-recent name. If
    // nothing changed, do not touch LVGL (avoids a per-frame repaint).
    uint32_t colors[proto::kMaxSessions] = {0};
    for (int i = 0; i < count; ++i)
        colors[i] = parse_color(s[order[i]].color);
    const char *name = (count > 0) ? s[order[0]].name : "";

    bool dirty = (count != s_last_count) || (strncmp(name, s_last_name, sizeof(s_last_name)) != 0);
    for (int i = 0; i < count && !dirty; ++i)
        if (colors[i] != s_last_colors[i]) dirty = true;
    if (!dirty) return;

    // Rings: show [0,count) in their controller color (outermost = order[0]);
    // hide [count, kMaxSessions).
    for (int i = 0; i < proto::kMaxSessions; ++i) {
        lv_obj_t *r = s_rings[i];
        if (!r) continue;
        if (i < count) {
            lv_obj_set_style_border_color(r, lv_color_hex(colors[i]), 0);
            lv_obj_remove_flag(r, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(r, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Pill: most-recent controller's name in its color, else hidden.
    if (count > 0) {
        lv_obj_set_style_bg_color(s_pill, lv_color_hex(colors[0]), 0);
        lv_label_set_text(s_pill_lbl, name);
        lv_obj_remove_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_pill, LV_OBJ_FLAG_HIDDEN);
    }

    s_last_count = count;
    for (int i = 0; i < proto::kMaxSessions; ++i)
        s_last_colors[i] = (i < count) ? colors[i] : 0;
    strncpy(s_last_name, name, sizeof(s_last_name) - 1);
    s_last_name[sizeof(s_last_name) - 1] = 0;
}

}  // namespace control_frame
}  // namespace ui
