#include "knob_menu.h"

#include <math.h>
#include <string.h>

namespace knob {

static const char *kModeState[kModeCount] = {"standby", "auto", "wind", "route"};

const char *mode_state_string(int idx) {
    if (idx < 0 || idx >= kModeCount) return "standby";
    return kModeState[idx];
}

const char *level_name(Level l) {
    switch (l) {
    case Level::Home:
        return "home";
    case Level::ModePicker:
        return "mode";
    case Level::SelectDisplay:
        return "display";
    case Level::SelectView:
        return "view";
    }
    return "?";
}

void init(Model &m) {
    m = Model{};
    m.pending_target_rad = NAN;
}

static double wrap_2pi(double r) {
    while (r < 0)
        r += 2 * M_PI;
    while (r >= 2 * M_PI)
        r -= 2 * M_PI;
    return r;
}

static double seed_target(const Model &m, const Inputs &in) {
    if (!isnan(m.pending_target_rad)) return m.pending_target_rad;
    if (!isnan(in.ap_target_rad)) return in.ap_target_rad;
    if (!isnan(in.heading_rad)) return in.heading_rad;
    return 0.0;
}

static Action adjust(Model &m, const Inputs &in, int sign, bool held) {
    int deg = (held ? kStepBigDeg : kStepSmallDeg) * sign;
    double t = wrap_2pi(seed_target(m, in) + deg * M_PI / 180.0);
    m.pending_target_rad = t;
    Action a;
    a.type = ActionType::ApSetTargetRad;
    a.arg_f = t;
    return a;
}

Action step(Model &m, const Inputs &in, Event ev, bool held) {
    Action a;  // NoAction
    switch (m.level) {
    case Level::Home:
        if (ev == Event::DetentCW) return adjust(m, in, +1, held);
        if (ev == Event::DetentCCW) return adjust(m, in, -1, held);
        break;
    default:
        break;
    }
    return a;
}

}  // namespace knob
