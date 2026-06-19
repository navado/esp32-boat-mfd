#include "bench_row.h"

#include <stdio.h>

namespace bench {

void BenchRow::reset() {
    sum_ = BenchSample{};
    n_ = 0;
}

void BenchRow::add(const BenchSample &s) {
    if (n_ == 0) {
        sum_ = s;
        n_ = 1;
        return;
    }
    // means: accumulate sum
    sum_.fps += s.fps;
    sum_.flush_avg_us += s.flush_avg_us;
    sum_.refresh_us += s.refresh_us;
    sum_.first_frame_us += s.first_frame_us;
    sum_.build_us += s.build_us;
    sum_.core0_idle_pct += s.core0_idle_pct;
    sum_.core1_idle_pct += s.core1_idle_pct;
    sum_.deltas_s += s.deltas_s;
    sum_.parsed_s += s.parsed_s;
    sum_.parse_avg_us += s.parse_avg_us;
    sum_.store_size += s.store_size;
    sum_.store_lookups_s += s.store_lookups_s;
    sum_.ws_frames_s += s.ws_frames_s;
    sum_.ws_bytes_s += s.ws_bytes_s;
    sum_.subscriptions += s.subscriptions;
    sum_.heap_lowwater_kb += s.heap_lowwater_kb;
    sum_.largest_block_kb += s.largest_block_kb;
    sum_.temp_c += s.temp_c;
    // peaks: take max
    if (s.flush_peak_us > sum_.flush_peak_us) sum_.flush_peak_us = s.flush_peak_us;
    if (s.lvgl_peak_us > sum_.lvgl_peak_us) sum_.lvgl_peak_us = s.lvgl_peak_us;
    if (s.loop_peak_us > sum_.loop_peak_us) sum_.loop_peak_us = s.loop_peak_us;
    // min stack high-water: take min (worst-case headroom)
    if (s.min_stack_b < sum_.min_stack_b) sum_.min_stack_b = s.min_stack_b;
    ++n_;
}

BenchSample BenchRow::result() const {
    BenchSample r{};
    if (n_ == 0) return r;
    const double d = (double)n_;
    r.fps = sum_.fps / d;
    r.flush_avg_us = sum_.flush_avg_us / d;
    r.refresh_us = sum_.refresh_us / d;
    r.first_frame_us = sum_.first_frame_us / d;
    r.build_us = sum_.build_us / d;
    r.core0_idle_pct = sum_.core0_idle_pct / d;
    r.core1_idle_pct = sum_.core1_idle_pct / d;
    r.deltas_s = sum_.deltas_s / d;
    r.parsed_s = sum_.parsed_s / d;
    r.parse_avg_us = sum_.parse_avg_us / d;
    r.store_size = sum_.store_size / d;
    r.store_lookups_s = sum_.store_lookups_s / d;
    r.ws_frames_s = sum_.ws_frames_s / d;
    r.ws_bytes_s = sum_.ws_bytes_s / d;
    r.subscriptions = sum_.subscriptions / d;
    r.heap_lowwater_kb = sum_.heap_lowwater_kb / d;
    r.largest_block_kb = sum_.largest_block_kb / d;
    r.temp_c = sum_.temp_c / d;
    // peaks / min carried through as-is
    r.flush_peak_us = sum_.flush_peak_us;
    r.lvgl_peak_us = sum_.lvgl_peak_us;
    r.loop_peak_us = sum_.loop_peak_us;
    r.min_stack_b = sum_.min_stack_b;
    return r;
}

int BenchRow::header(char *out, size_t cap) {
    return snprintf(out, cap,
                    "screen,mode,fps,flush_avg_us,flush_peak_us,refresh_us,first_frame_us,build_us,"
                    "lvgl_peak_us,"
                    "core0_idle_pct,core1_idle_pct,loop_peak_us,deltas_s,parsed_s,parse_avg_us,"
                    "store_size,store_lookups_s,ws_frames_s,ws_bytes_s,subscriptions,"
                    "heap_lowwater_kb,largest_block_kb,min_stack_b,temp_c");
}

int BenchRow::format(const char *label, char *out, size_t cap) const {
    BenchSample r = result();
    return snprintf(out, cap,
                    "%s,%.1f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.1f,%.1f,%.0f,%.0f,%.1f,%.0f,"
                    "%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.1f",
                    label ? label : "", r.fps, r.flush_avg_us, r.flush_peak_us, r.refresh_us,
                    r.first_frame_us, r.build_us, r.lvgl_peak_us, r.core0_idle_pct,
                    r.core1_idle_pct, r.loop_peak_us, r.deltas_s, r.parsed_s, r.parse_avg_us,
                    r.store_size, r.store_lookups_s, r.ws_frames_s, r.ws_bytes_s, r.subscriptions,
                    r.heap_lowwater_kb, r.largest_block_kb, r.min_stack_b, r.temp_c);
}

}  // namespace bench
