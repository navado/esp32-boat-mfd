#include "screens.h"
#include "ui_theme.h"
#include "ui_data.h"
#include "ui_layouts.h"
#include "signalk.h"
#include "board_pins.h"

// Depth screen delegated to the QuadGrid template. Matches the editor's
// `depthTempScreen()` preset: numeric DEPTH / numeric BELOW K / numeric
// H2O TEMP / trend DEPTH 5m. Trend falls back to numeric in the device
// painter today (sparkline TODO).

namespace ui::depth {

static lv_obj_t *s_root = nullptr;

static const ui::layouts::MetricBinding s_tiles[] = {
    {"depth",
     "DEPTH",
     "m",
     ui::layouts::MetricSource::Depth_m,
     0x39d98a /*good*/,
     nullptr,
     0,
     {},
     ui::layouts::WidgetKind::Numeric},
    {"depthKeel",
     "BELOW K",
     "m",
     ui::layouts::MetricSource::Depth_m,
     0x57c7d8 /*accent*/,
     nullptr,
     0,
     {},
     ui::layouts::WidgetKind::Numeric},
    {"waterTemp",
     "H2O TEMP",
     "C",
     ui::layouts::MetricSource::WaterTemp_C,
     0x288cff /*tide*/,
     nullptr,
     0,
     {},
     ui::layouts::WidgetKind::Numeric},
    {"depthTrend",
     "DEPTH 5m",
     "m",
     ui::layouts::MetricSource::Depth_m,
     0x52736f /*grid*/,
     nullptr,
     0,
     {},
     ui::layouts::WidgetKind::Trend},
};

static const ui::layouts::ScreenVariantSpec s_spec = {
    "depth",
    "Depth",
    ui::layouts::TemplateId::QuadGrid,
    s_tiles,
    sizeof(s_tiles) / sizeof(s_tiles[0]),
    0,
};

lv_obj_t *build(lv_obj_t *parent) {
    s_root = ui::layouts::create(parent, s_spec);
    return s_root;
}

void refresh() {
    if (!s_root) return;
    sk::Data d;
    sk::copyData(d);
    ui::layouts::update(s_root, s_spec, d);
}

}  // namespace ui::depth
