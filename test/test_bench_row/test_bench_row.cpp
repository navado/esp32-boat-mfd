#include "bench_row.h"

#include <string.h>
#include <unity.h>

using bench::BenchRow;
using bench::BenchSample;

void setUp() {
}
void tearDown() {
}

static void test_empty_result_is_zero() {
    BenchRow r;
    TEST_ASSERT_EQUAL_INT(0, r.count());
    BenchSample s = r.result();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, s.fps);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, s.ws_bytes_s);
}

static void test_mean_of_level_fields() {
    BenchRow r;
    BenchSample a{};
    a.fps = 10;
    a.ws_bytes_s = 1000;
    a.refresh_us = 200;
    BenchSample b{};
    b.fps = 20;
    b.ws_bytes_s = 3000;
    b.refresh_us = 400;
    r.add(a);
    r.add(b);
    TEST_ASSERT_EQUAL_INT(2, r.count());
    BenchSample m = r.result();
    TEST_ASSERT_EQUAL_DOUBLE(15.0, m.fps);
    TEST_ASSERT_EQUAL_DOUBLE(2000.0, m.ws_bytes_s);
    TEST_ASSERT_EQUAL_DOUBLE(300.0, m.refresh_us);
}

static void test_peak_fields_take_max() {
    BenchRow r;
    BenchSample a{};
    a.flush_peak_us = 800;
    a.loop_peak_us = 1200;
    BenchSample b{};
    b.flush_peak_us = 500;  // lower
    b.loop_peak_us = 1500;  // higher
    r.add(a);
    r.add(b);
    BenchSample m = r.result();
    TEST_ASSERT_EQUAL_DOUBLE(800.0, m.flush_peak_us);  // max, not mean
    TEST_ASSERT_EQUAL_DOUBLE(1500.0, m.loop_peak_us);
}

static void test_first_frame_and_build_are_meaned() {
    BenchRow r;
    BenchSample a{};
    a.first_frame_us = 1000;
    a.build_us = 8000;
    BenchSample b{};
    b.first_frame_us = 2000;
    b.build_us = 8000;
    r.add(a);
    r.add(b);
    BenchSample m = r.result();
    TEST_ASSERT_EQUAL_DOUBLE(1500.0, m.first_frame_us);
    TEST_ASSERT_EQUAL_DOUBLE(8000.0, m.build_us);
}

static void test_min_stack_takes_minimum() {
    BenchRow r;
    BenchSample a{};
    a.min_stack_b = 4096;
    BenchSample b{};
    b.min_stack_b = 2048;  // worse headroom
    r.add(a);
    r.add(b);
    TEST_ASSERT_EQUAL_DOUBLE(2048.0, r.result().min_stack_b);
}

static void test_reset_clears() {
    BenchRow r;
    BenchSample a{};
    a.fps = 5;
    r.add(a);
    r.reset();
    TEST_ASSERT_EQUAL_INT(0, r.count());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, r.result().fps);
}

static void test_format_row_and_header() {
    char buf[512];
    int hn = BenchRow::header(buf, sizeof(buf));
    TEST_ASSERT_TRUE(hn > 0);
    TEST_ASSERT_NOT_NULL(strstr(buf, "screen,mode,fps"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "first_frame_us"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "build_us"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "ws_bytes_s"));

    BenchRow r;
    BenchSample a{};
    a.fps = 12.0;
    a.ws_bytes_s = 2500;
    r.add(a);
    int n = r.format("nav/typed", buf, sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL_INT(0, strncmp(buf, "nav/typed,12.0,", 15));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_result_is_zero);
    RUN_TEST(test_mean_of_level_fields);
    RUN_TEST(test_peak_fields_take_max);
    RUN_TEST(test_first_frame_and_build_are_meaned);
    RUN_TEST(test_min_stack_takes_minimum);
    RUN_TEST(test_reset_clears);
    RUN_TEST(test_format_row_and_header);
    return UNITY_END();
}
