#pragma once

// Pure accumulator + formatter for one row of the on-device benchmark sweep
// (one screen, one render mode). Per-second samples are added with add(); mean
// fields are averaged, *_peak fields take the maximum. Host-testable (no
// Arduino/LVGL/FreeRTOS deps); the device sampler fills BenchSample each tick.

#include <stddef.h>

namespace bench {

// One per-second snapshot of the device's metrics.
struct BenchSample {
    // render
    double fps = 0;
    double flush_avg_us = 0;
    double flush_peak_us = 0;  // peak
    double refresh_us = 0;
    double first_frame_us = 0;  // screen show -> first flush (load+first paint latency)
    double build_us = 0;        // cold build_fn() duration for this screen (one-time)
    double lvgl_peak_us = 0;    // peak
    // cpu
    double core0_idle_pct = 0;
    double core1_idle_pct = 0;
    double loop_peak_us = 0;  // peak
    // throughput
    double deltas_s = 0;
    double parsed_s = 0;
    double parse_avg_us = 0;
    double store_size = 0;
    double store_lookups_s = 0;
    // network
    double ws_frames_s = 0;
    double ws_bytes_s = 0;
    double subscriptions = 0;
    // memory
    double heap_lowwater_kb = 0;
    double largest_block_kb = 0;
    double min_stack_b = 0;  // min over time (peak pressure) -> use min
    // thermal
    double temp_c = 0;
};

class BenchRow {
  public:
    void reset();
    void add(const BenchSample &s);
    int count() const { return n_; }

    // Averages for level/rate fields, max for *_peak fields, min for min_stack_b.
    // Returns a zeroed sample when count()==0.
    BenchSample result() const;

    // Fixed CSV header (no trailing newline). Returns chars written (excl NUL).
    static int header(char *out, size_t cap);

    // CSV row: "<label>,<fps>,<flush_avg>,..." Returns chars written (excl NUL).
    int format(const char *label, char *out, size_t cap) const;

  private:
    BenchSample sum_;  // running sum (means) / max (peaks) / min (min_stack)
    int n_ = 0;
};

}  // namespace bench
