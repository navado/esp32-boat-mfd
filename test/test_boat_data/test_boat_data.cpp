#include <unity.h>
#include <cmath>

#include "boat_data.h"

using namespace boat;

void setUp(void) { reset_all(); }
void tearDown(void) {}

static void test_defaults() {
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_TRUE(std::isnan(s.sog_mps.value));
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::None),
                      static_cast<int>(s.sog_mps.source));
    TEST_ASSERT_EQUAL_UINT32(0, s.sog_mps.updated_ms);
}

static void test_rank_of_default_priority() {
    Priority p;
    TEST_ASSERT_EQUAL_UINT8(0, rank_of(p, SourceKind::Nmea2000));
    TEST_ASSERT_EQUAL_UINT8(1, rank_of(p, SourceKind::NmeaWifi));
    TEST_ASSERT_EQUAL_UINT8(2, rank_of(p, SourceKind::SignalK));
    TEST_ASSERT_EQUAL_UINT8(3, rank_of(p, SourceKind::Demo));
    TEST_ASSERT_EQUAL_UINT8(255, rank_of(p, SourceKind::None));
}

static void test_fresh_window() {
    Field f;
    f.value = 1.0;
    f.source = SourceKind::SignalK;
    f.updated_ms = 100;
    TEST_ASSERT_TRUE(fresh(f, 200, 1000));
    TEST_ASSERT_FALSE(fresh(f, 2000, 1000));
    TEST_ASSERT_FALSE(fresh(f, 100, 0));
    Field empty;
    TEST_ASSERT_FALSE(fresh(empty, 100, 1000));
}

static void test_value_or_nan() {
    Field f;
    f.value = 4.13;
    f.source = SourceKind::NmeaWifi;
    f.updated_ms = 100;
    TEST_ASSERT_EQUAL_DOUBLE(4.13, value_or_nan(f, 200, 1000));
    TEST_ASSERT_TRUE(std::isnan(value_or_nan(f, 5000, 1000)));
}

static void test_signalk_only_publish() {
    publish(&Snapshot::sog_mps, SourceKind::SignalK, 1000, 3.5);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(3.5, s.sog_mps.value);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::SignalK),
                      static_cast<int>(s.sog_mps.source));
}

static void test_priority_higher_wins_when_both_fresh() {
    publish(&Snapshot::sog_mps, SourceKind::SignalK, 1000, 3.5);
    bool accepted = publish(&Snapshot::sog_mps, SourceKind::NmeaWifi, 1100, 4.0);
    TEST_ASSERT_TRUE(accepted);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, s.sog_mps.value);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::NmeaWifi),
                      static_cast<int>(s.sog_mps.source));
}

static void test_lower_priority_rejected_when_higher_fresh() {
    publish(&Snapshot::sog_mps, SourceKind::Nmea2000, 1000, 4.0);
    bool accepted = publish(&Snapshot::sog_mps, SourceKind::SignalK, 1100, 3.5);
    TEST_ASSERT_FALSE(accepted);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, s.sog_mps.value);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::Nmea2000),
                      static_cast<int>(s.sog_mps.source));
}

static void test_lower_priority_accepted_when_higher_stale() {
    // Default n2k timeout = 2000ms.
    publish(&Snapshot::sog_mps, SourceKind::Nmea2000, 1000, 4.0);
    bool accepted = publish(&Snapshot::sog_mps, SourceKind::SignalK, 5000, 3.5);
    TEST_ASSERT_TRUE(accepted);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(3.5, s.sog_mps.value);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::SignalK),
                      static_cast<int>(s.sog_mps.source));
}

static void test_custom_priority_reorders() {
    Priority p;
    p.order[0] = SourceKind::SignalK;
    p.order[1] = SourceKind::NmeaWifi;
    p.order[2] = SourceKind::Nmea2000;
    p.order[3] = SourceKind::Demo;
    p.order[4] = SourceKind::None;
    set_priority(p);
    publish(&Snapshot::sog_mps, SourceKind::SignalK, 1000, 3.5);
    bool accepted = publish(&Snapshot::sog_mps, SourceKind::Nmea2000, 1100, 4.0);
    TEST_ASSERT_FALSE(accepted);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(3.5, s.sog_mps.value);
}

static void test_should_accept_pure() {
    Priority p;
    Timeouts t;
    // Same source overwriting itself is fine.
    TEST_ASSERT_TRUE(should_accept(SourceKind::SignalK, SourceKind::SignalK,
                                   100, 200, p, t));
    // Higher priority always wins when fresh.
    TEST_ASSERT_TRUE(should_accept(SourceKind::Nmea2000, SourceKind::SignalK,
                                   100, 200, p, t));
    // None never accepted.
    TEST_ASSERT_FALSE(should_accept(SourceKind::None, SourceKind::SignalK,
                                    100, 200, p, t));
    // Lower priority rejected when higher fresh.
    TEST_ASSERT_FALSE(should_accept(SourceKind::SignalK, SourceKind::Nmea2000,
                                    100, 200, p, t));
    // Lower priority accepted when higher stale.
    TEST_ASSERT_TRUE(should_accept(SourceKind::SignalK, SourceKind::Nmea2000,
                                   100, 5000, p, t));
}

static void test_autopilot_state_publish() {
    publish_autopilot_state(SourceKind::SignalK, 1000, "auto");
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_STRING("auto", s.autopilot_state);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::SignalK),
                      static_cast<int>(s.autopilot_state_source));

    // Higher priority overrides.
    publish_autopilot_state(SourceKind::Nmea2000, 1100, "wind");
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_STRING("wind", s.autopilot_state);

    // Lower priority rejected.
    publish_autopilot_state(SourceKind::SignalK, 1200, "standby");
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_STRING("wind", s.autopilot_state);
}

static void test_independent_fields() {
    publish(&Snapshot::sog_mps, SourceKind::Nmea2000, 1000, 4.0);
    publish(&Snapshot::depth_m, SourceKind::SignalK, 1000, 12.0);
    Snapshot s;
    copy_snapshot(s);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, s.sog_mps.value);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, s.depth_m.value);
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::Nmea2000),
                      static_cast<int>(s.sog_mps.source));
    TEST_ASSERT_EQUAL(static_cast<int>(SourceKind::SignalK),
                      static_cast<int>(s.depth_m.source));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_rank_of_default_priority);
    RUN_TEST(test_fresh_window);
    RUN_TEST(test_value_or_nan);
    RUN_TEST(test_signalk_only_publish);
    RUN_TEST(test_priority_higher_wins_when_both_fresh);
    RUN_TEST(test_lower_priority_rejected_when_higher_fresh);
    RUN_TEST(test_lower_priority_accepted_when_higher_stale);
    RUN_TEST(test_custom_priority_reorders);
    RUN_TEST(test_should_accept_pure);
    RUN_TEST(test_autopilot_state_publish);
    RUN_TEST(test_independent_fields);
    return UNITY_END();
}
