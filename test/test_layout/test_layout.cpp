#include <unity.h>
#include <cstring>
#include "layout.h"

using namespace layout;

void setUp(void) {
}
void tearDown(void) {
}

static const char *MINIMAL_JSON =
    "{"
    "\"version\":1,"
    "\"settings\":{\"default_screen\":\"dash\",\"demo_period_ms\":2500},"
    "\"screens\":[{"
    "  \"id\":\"dash\",\"title\":\"Dashboard\",\"type\":\"quadrants\","
    "  \"tiles\":["
    "    {\"id\":\"w\",\"title\":\"WIND\",\"type\":\"wind\","
    "     \"paths\":{\"awa\":\"environment.wind.angleApparent\","
    "                \"aws\":\"environment.wind.speedApparent\"}},"
    "    {\"id\":\"n\",\"title\":\"NAV\",\"type\":\"nav\"}"
    "  ]"
    "}],"
    "\"alarms\":["
    "  {\"id\":\"shallow\",\"path\":\"environment.depth.belowTransducer\","
    "   \"level\":\"alarm\",\"lt\":3.0,\"message\":\"SHALLOW WATER\"}"
    "]"
    "}";

static void test_parse_minimal() {
    Config c;
    int rc = parse(MINIMAL_JSON, strlen(MINIMAL_JSON), c);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(1, c.version);
    TEST_ASSERT_EQUAL_STRING("dash", c.settings.default_screen);
    TEST_ASSERT_EQUAL_UINT32(2500, c.settings.demo_period_ms);
    TEST_ASSERT_EQUAL(1, c.screen_count);
    TEST_ASSERT_EQUAL_STRING("dash", c.screens[0].id);
    TEST_ASSERT_EQUAL(SCREEN_QUADRANTS, c.screens[0].type);
    TEST_ASSERT_EQUAL(2, c.screens[0].tile_count);
}

static void test_tile_types_and_paths() {
    Config c;
    parse(MINIMAL_JSON, strlen(MINIMAL_JSON), c);
    const Tile &t = c.screens[0].tiles[0];
    TEST_ASSERT_EQUAL_STRING("w", t.id);
    TEST_ASSERT_EQUAL_STRING("WIND", t.title);
    TEST_ASSERT_EQUAL(TILE_WIND, t.type);
    TEST_ASSERT_EQUAL(2, t.path_count);
    // Order of keys in JSON objects is insertion-order in ArduinoJson 7.
    TEST_ASSERT_EQUAL_STRING("awa", t.paths[0].key);
    TEST_ASSERT_EQUAL_STRING("environment.wind.angleApparent", t.paths[0].path);
    TEST_ASSERT_EQUAL_STRING("aws", t.paths[1].key);
    TEST_ASSERT_EQUAL(TILE_NAV, c.screens[0].tiles[1].type);
    TEST_ASSERT_EQUAL(0, c.screens[0].tiles[1].path_count);
}

static void test_parse_alarm_rule() {
    Config c;
    parse(MINIMAL_JSON, strlen(MINIMAL_JSON), c);
    TEST_ASSERT_EQUAL(1, c.alarm_count);
    const AlarmRule &a = c.alarms[0];
    TEST_ASSERT_EQUAL_STRING("shallow", a.id);
    TEST_ASSERT_EQUAL_STRING("environment.depth.belowTransducer", a.path);
    TEST_ASSERT_EQUAL(ALARM_LVL_ALARM, a.level);
    TEST_ASSERT_TRUE(a.has_lt);
    TEST_ASSERT_FALSE(a.has_gt);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.0, a.lt);
    TEST_ASSERT_EQUAL_STRING("SHALLOW WATER", a.message);
}

static void test_unknown_types_default_to_unknown_enum() {
    const char *j = "{\"screens\":[{\"id\":\"x\",\"type\":\"made-up\","
                    "\"tiles\":[{\"id\":\"y\",\"type\":\"also-made-up\"}]}]}";
    Config c;
    parse(j, strlen(j), c);
    TEST_ASSERT_EQUAL(1, c.screen_count);
    TEST_ASSERT_EQUAL(SCREEN_UNKNOWN, c.screens[0].type);
    TEST_ASSERT_EQUAL(TILE_UNKNOWN, c.screens[0].tiles[0].type);
}

static void test_find_screen() {
    Config c;
    parse(MINIMAL_JSON, strlen(MINIMAL_JSON), c);
    const Screen *s = find_screen(c, "dash");
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL(SCREEN_QUADRANTS, s->type);
    TEST_ASSERT_NULL(find_screen(c, "nonexistent"));
}

static void test_invalid_json_returns_error() {
    const char *bad = "{not valid";
    Config c;
    int rc = parse(bad, strlen(bad), c);
    TEST_ASSERT_LESS_THAN(0, rc);
}

static void test_empty_object_is_valid() {
    const char *j = "{}";
    Config c;
    int rc = parse(j, strlen(j), c);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, c.screen_count);
    TEST_ASSERT_EQUAL(0, c.alarm_count);
    TEST_ASSERT_EQUAL_STRING("", c.settings.default_screen);
}

static void test_too_many_screens_are_clamped() {
    char buf[4096];
    int n = snprintf(buf, sizeof(buf), "{\"screens\":[");
    for (size_t i = 0; i < MAX_SCREENS + 3; ++i) {
        n += snprintf(buf + n, sizeof(buf) - n, "%s{\"id\":\"s%u\",\"type\":\"quadrants\"}",
                      i == 0 ? "" : ",", (unsigned)i);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "]}");
    Config c;
    int rc = parse(buf, strlen(buf), c);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(MAX_SCREENS, c.screen_count);
}

static void test_long_strings_are_truncated_safely() {
    const char *j =
        "{\"screens\":[{\"id\":\"thisIsADeliberatelyLongIdentifierThatExceedsTheFixedBufferOf32\","
        "\"type\":\"quadrants\"}]}";
    Config c;
    parse(j, strlen(j), c);
    TEST_ASSERT_EQUAL(1, c.screen_count);
    TEST_ASSERT_EQUAL(STR_LEN - 1, strlen(c.screens[0].id));  // truncated, NUL-terminated
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_minimal);
    RUN_TEST(test_tile_types_and_paths);
    RUN_TEST(test_parse_alarm_rule);
    RUN_TEST(test_unknown_types_default_to_unknown_enum);
    RUN_TEST(test_find_screen);
    RUN_TEST(test_invalid_json_returns_error);
    RUN_TEST(test_empty_object_is_valid);
    RUN_TEST(test_too_many_screens_are_clamped);
    RUN_TEST(test_long_strings_are_truncated_safely);
    return UNITY_END();
}
