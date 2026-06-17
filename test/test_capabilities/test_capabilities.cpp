#include "capabilities.h"

#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

void setUp() {
}
void tearDown() {
}

static void test_version_and_limits() {
    JsonDocument doc;
    capabilities::build_manifest(doc.to<JsonObject>());
    TEST_ASSERT_EQUAL_INT(capabilities::MANIFEST_VERSION, doc["version"].as<int>());
    TEST_ASSERT_EQUAL_INT(8, doc["maxViews"].as<int>());
    TEST_ASSERT_EQUAL_INT(4, doc["maxTilesPerScreen"].as<int>());
    TEST_ASSERT_EQUAL_STRING("open", doc["paths"].as<const char *>());
}

static void test_view_types_present() {
    JsonDocument doc;
    capabilities::build_manifest(doc.to<JsonObject>());
    JsonObject vts = doc["viewTypes"];
    for (const char *t :
         {"numeric", "compass", "windCircle", "gauge", "bar", "trend", "text", "control"}) {
        TEST_ASSERT_TRUE_MESSAGE(vts[t].is<JsonObject>(), t);
    }
    // numeric: single value path + the standard attrs incl. unit + color.
    JsonArray np = vts["numeric"]["paths"];
    TEST_ASSERT_EQUAL_INT(1, np.size());
    TEST_ASSERT_EQUAL_STRING("value", np[0].as<const char *>());
    // windCircle binds value + dir.
    JsonArray wp = vts["windCircle"]["paths"];
    TEST_ASSERT_EQUAL_INT(2, wp.size());
    TEST_ASSERT_EQUAL_STRING("dir", wp[1].as<const char *>());
}

static void test_gauge_has_range_and_zones() {
    JsonDocument doc;
    capabilities::build_manifest(doc.to<JsonObject>());
    JsonArray attrs = doc["viewTypes"]["gauge"]["attrs"];
    bool has_range = false, has_zones = false;
    for (JsonVariant a : attrs) {
        if (!strcmp(a.as<const char *>(), "range")) has_range = true;
        if (!strcmp(a.as<const char *>(), "zones")) has_zones = true;
    }
    TEST_ASSERT_TRUE(has_range);
    TEST_ASSERT_TRUE(has_zones);
}

static void test_font_sizes_and_units() {
    JsonDocument doc;
    capabilities::build_manifest(doc.to<JsonObject>());
    JsonArray fs = doc["fontSizes"];
    TEST_ASSERT_TRUE(fs.size() >= 2);
    TEST_ASSERT_EQUAL_INT(14, fs[0].as<int>());  // DEFAULT_SIZES[0]
    // speed unit family includes kn + m/s.
    JsonArray speed = doc["units"]["speed"];
    TEST_ASSERT_EQUAL_STRING("kn", speed[0].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("m/s", speed[1].as<const char *>());
}

static void test_controls_and_themes() {
    JsonDocument doc;
    capabilities::build_manifest(doc.to<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("autopilot", doc["controls"][0].as<const char *>());
    JsonArray themes = doc["themes"];
    TEST_ASSERT_EQUAL_INT(3, themes.size());
    TEST_ASSERT_EQUAL_STRING("day", themes[0].as<const char *>());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_version_and_limits);
    RUN_TEST(test_view_types_present);
    RUN_TEST(test_gauge_has_range_and_zones);
    RUN_TEST(test_font_sizes_and_units);
    RUN_TEST(test_controls_and_themes);
    return UNITY_END();
}
