#include "board.h"

namespace ui {

LayoutContext layout_context() {
    board::Geometry g = board::geometry();
    const uint16_t usable_w = g.usable_width ? g.usable_width : g.width_px;
    const uint16_t usable_h = g.usable_height ? g.usable_height : g.height_px;
    LayoutContext c{};
    c.w = g.width_px;
    c.h = g.height_px;
    c.short_side = usable_w < usable_h ? usable_w : usable_h;
    c.long_side = usable_w > usable_h ? usable_w : usable_h;
    c.square = g.square;
    c.landscape = !g.square && g.width_px > g.height_px;
    c.wide = g.layout_class == board::LayoutClass::LandscapeWide || g.diagonal_tenths_in >= 70 ||
             g.width_px >= 1024;
    const bool hdpi = g.density_class == board::DensityClass::Hdpi;
    c.margin = hdpi ? 16 : 8;
    c.gap = hdpi ? 8 : 4;
    c.touch_min = hdpi ? 56 : 44;
    return c;
}

}  // namespace ui
