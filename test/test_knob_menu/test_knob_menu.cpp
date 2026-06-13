#include <unity.h>
#include <math.h>
#include <string.h>

#include "knob_menu.h"

using namespace knob;

void setUp(void) {
}
void tearDown(void) {
}

static Inputs base_inputs() {
    Inputs in;
    in.ap_state = "standby";
    in.ap_target_rad = NAN;
    in.heading_rad = M_PI;  // 180 deg
    in.display_count = 0;
    in.view_count = 0;
    return in;
}

static void test_home_cw_small_step_presets_target_from_heading() {
    Model m;
    init(m);
    Inputs in = base_inputs();
    Action a = step(m, in, Event::DetentCW, /*held=*/false);
    TEST_ASSERT_EQUAL_INT((int)ActionType::ApSetTargetRad, (int)a.type);
    // seeded from heading 180, +1 deg -> 181 deg
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 181.0 * M_PI / 180.0, a.arg_f);
}

static void test_home_cw_big_step_when_held() {
    Model m;
    init(m);
    Inputs in = base_inputs();
    Action a = step(m, in, Event::DetentCW, /*held=*/true);
    TEST_ASSERT_EQUAL_INT((int)ActionType::ApSetTargetRad, (int)a.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 185.0 * M_PI / 180.0, a.arg_f);
}

static void test_home_ccw_wraps_below_zero() {
    Model m;
    init(m);
    Inputs in = base_inputs();
    in.heading_rad = 0.0;
    Action a = step(m, in, Event::DetentCCW, /*held=*/false);
    // 0 - 1 deg -> 359 deg
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 359.0 * M_PI / 180.0, a.arg_f);
}

static void test_home_adjust_accumulates_into_pending() {
    Model m;
    init(m);
    Inputs in = base_inputs();
    step(m, in, Event::DetentCW, false);             // 181
    Action a = step(m, in, Event::DetentCW, false);  // 182
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 182.0 * M_PI / 180.0, a.arg_f);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_home_cw_small_step_presets_target_from_heading);
    RUN_TEST(test_home_cw_big_step_when_held);
    RUN_TEST(test_home_ccw_wraps_below_zero);
    RUN_TEST(test_home_adjust_accumulates_into_pending);
    return UNITY_END();
}
