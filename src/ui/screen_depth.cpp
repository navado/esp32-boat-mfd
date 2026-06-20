#include "screens.h"
#include "ui_screens.h"
#include "ui_theme.h"
#include "ui_data.h"
#include "ui_layouts.h"
#include "signalk.h"
#include "board_pins.h"

// Depth screen delegated to the HeroPlus template: one large HERO value with a
// sub-group of secondary stats beneath it.
//
//   HERO   = Depth below keel (DepthKeel_m, "m")
//   sub 1  = Water temperature (WaterTemp_C, "C")
//   sub 2  = Speed over ground (SOG_kn, "kn")
//   sub 3  = True wind angle (TWA, deg, port/stbd suffix)
//
// NOTE on the "true-wind compass" sub-tile: HeroPlus renders its sub-group from
// metrics[0].extras[] as plain "<label> <value>" text rows (Numeric only) — the
// template has no per-extra WidgetKind, so a WidgetKind::Compass sub-tile is NOT
// supported here. The 3rd sub therefore degrades to a numeric TWA tile (true
// wind angle, the same datum a wind compass would show). Promoting it to a real
// compass would require a hand-built screen (ui::build_compass) or extending the
// HeroPlus template with a compass sub-slot.

namespace ui::depth {

static lv_obj_t *s_root = nullptr;

static const ui::layouts::MetricBinding s_tiles[] = {
    {"depthKeel",
     "BELOW KEEL",
     "m",
     ui::layouts::MetricSource::DepthKeel_m,
     0x57c7d8 /*accent*/,
     nullptr,
     3,
     {
         {"TEMP", ui::layouts::MetricSource::WaterTemp_C},
         {"SOG", ui::layouts::MetricSource::SOG_kn},
         {"TWA", ui::layouts::MetricSource::TWA_deg},
     },
     ui::layouts::WidgetKind::Numeric},
};

static const ui::layouts::ScreenVariantSpec s_spec = {
    "depth",
    "Depth",
    ui::layouts::TemplateId::HeroPlus,
    s_tiles,
    sizeof(s_tiles) / sizeof(s_tiles[0]),
    0,
};

static void collect_paths(sk::SubscriptionSet &out) {
    ui::layouts::collect_paths(s_spec, out);
}

lv_obj_t *build(lv_obj_t *parent) {
    s_root = ui::layouts::create(parent, s_spec);
    ui::set_screen_collect_paths(s_spec.screen_id, collect_paths);
    return s_root;
}

void refresh() {
    if (!s_root) return;
    sk::Data d;
    sk::copyData(d);
    ui::layouts::update(s_root, s_spec, d);
}

}  // namespace ui::depth
