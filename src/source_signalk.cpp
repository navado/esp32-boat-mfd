#include "source_signalk.h"

#include <math.h>

namespace boat {

namespace {

// Helper: publish if `v` is finite. Returns 1 on accept, 0 otherwise.
inline int pub(Field Snapshot::*field, uint32_t now_ms, double v) {
    if (!isfinite(v)) return 0;
    return publish(field, SourceKind::SignalK, now_ms, v) ? 1 : 0;
}

}  // namespace

int bridge_signalk_into_boat(const sk::Data &sk, uint32_t now_ms) {
    int n = 0;
    n += pub(&Snapshot::lat_deg,             now_ms, sk.lat);
    n += pub(&Snapshot::lon_deg,             now_ms, sk.lon);
    n += pub(&Snapshot::sog_mps,             now_ms, sk.sog);
    n += pub(&Snapshot::stw_mps,             now_ms, sk.stw);
    n += pub(&Snapshot::cog_true_rad,        now_ms, sk.cogTrue);
    n += pub(&Snapshot::heading_true_rad,    now_ms, sk.headingTrue);
    n += pub(&Snapshot::awa_rad,             now_ms, sk.awa);
    n += pub(&Snapshot::aws_mps,             now_ms, sk.aws);
    n += pub(&Snapshot::twa_rad,             now_ms, sk.twa);
    n += pub(&Snapshot::tws_mps,             now_ms, sk.tws);
    n += pub(&Snapshot::depth_m,             now_ms, sk.depth);
    n += pub(&Snapshot::water_temp_k,        now_ms, sk.waterTemp);
    n += pub(&Snapshot::battery_v,           now_ms, sk.battVoltage);
    n += pub(&Snapshot::battery_soc,         now_ms, sk.battSoc);
    n += pub(&Snapshot::tank_fuel,           now_ms, sk.tankFuel);
    n += pub(&Snapshot::tank_water,          now_ms, sk.tankWater);
    n += pub(&Snapshot::xte_m,               now_ms, sk.xte);
    n += pub(&Snapshot::cts_rad,             now_ms, sk.cts);
    n += pub(&Snapshot::btw_rad,             now_ms, sk.btw);
    n += pub(&Snapshot::dtw_m,               now_ms, sk.dtw);
    n += pub(&Snapshot::vmg_mps,             now_ms, sk.vmg);
    n += pub(&Snapshot::autopilot_target_rad, now_ms, sk.apTargetHdg);
    n += pub(&Snapshot::current_set_rad,     now_ms, sk.currentSetTrue);
    n += pub(&Snapshot::current_drift_mps,   now_ms, sk.currentDrift);
    if (sk.apState[0] != 0) {
        if (publish_autopilot_state(SourceKind::SignalK, now_ms, sk.apState)) {
            n++;
        }
    }
    return n;
}

}  // namespace boat
