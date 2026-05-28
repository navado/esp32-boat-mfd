#include <unity.h>
#include <ArduinoJson.h>
#include <cstring>
#include <string>

#include "manager_config.h"

using namespace manager_config;

void setUp(void) {}
void tearDown(void) {}

static JsonDocument _doc;

static JsonObjectConst parse_json(const char *s) {
    _doc.clear();
    DeserializationError e = deserializeJson(_doc, s);
    TEST_ASSERT_TRUE_MESSAGE(e == DeserializationError::Ok, "bad test JSON");
    return _doc.as<JsonObjectConst>();
}

static void test_widget_type_round_trip() {
    const char *names[] = {"numeric", "text", "gauge", "compass",
                           "windRose", "trend", "bar", "button", "autopilot"};
    for (const char *n : names) {
        WidgetType t = widget_type_from_string(n);
        TEST_ASSERT_TRUE_MESSAGE(t != WidgetType::Unknown, n);
        TEST_ASSERT_EQUAL_STRING(n, widget_type_to_string(t));
    }
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WidgetType::Unknown),
                          static_cast<int>(widget_type_from_string("nope")));
}

static void test_minimal_widgets_parses() {
    auto cfg = parse_json(R"({
        "widgets": {
            "defaults": {"valueFontSize": 34},
            "items": {
                "sog": {
                    "type": "numeric",
                    "title": "SOG",
                    "path": "navigation.speedOverGround",
                    "unit": "kn",
                    "precision": 1,
                    "fontSize": 42
                }
            }
        }
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_TRUE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_UINT8(1, plan.widget_count);
    TEST_ASSERT_EQUAL_STRING("sog", plan.widgets[0].id);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WidgetType::Numeric),
                          static_cast<int>(plan.widgets[0].type));
    TEST_ASSERT_EQUAL_STRING("SOG", plan.widgets[0].title);
    TEST_ASSERT_EQUAL_UINT16(42, plan.widgets[0].style.font_size);
    TEST_ASSERT_EQUAL_UINT16(34, plan.defaults.value_font_size);
}

static void test_unsupported_widget_type_rejected() {
    auto cfg = parse_json(R"({
        "widgets": {"items": {"chart": {"type": "map"}}}
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_FALSE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParseCode::UnsupportedWidgetType),
                          static_cast<int>(err.code));
    TEST_ASSERT_TRUE(strstr(err.path, "chart") != nullptr);
}

static void test_display_mismatch_rejected() {
    auto cfg = parse_json(R"({"display": {"width": 800, "height": 480}})");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_FALSE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParseCode::DisplayMismatch),
                          static_cast<int>(err.code));
}

static void test_screen_references_missing_widget_rejected() {
    auto cfg = parse_json(R"({
        "widgets": {"items": {"sog": {"type": "numeric"}}},
        "layout": {
            "screens": [{
                "id": "dashboard",
                "type": "grid",
                "tiles": [{"widget": "missing", "area": {"col":0,"row":0}}]
            }]
        }
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_FALSE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParseCode::MissingWidget),
                          static_cast<int>(err.code));
}

static void test_screen_with_grid_layout_accepted() {
    auto cfg = parse_json(R"({
        "widgets": {"items": {
            "sog": {"type": "numeric", "title": "SOG"},
            "depth": {"type": "numeric", "title": "DPT"}
        }},
        "layout": {
            "screens": [{
                "id": "dashboard",
                "type": "grid",
                "tiles": [
                    {"widget": "sog",   "area": {"col":0,"row":0,"colSpan":2,"rowSpan":1}},
                    {"widget": "depth", "area": {"col":2,"row":0}}
                ]
            }]
        }
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_TRUE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_UINT8(2, plan.widget_count);
    TEST_ASSERT_EQUAL_UINT8(1, plan.screen_count);
    TEST_ASSERT_EQUAL_STRING("dashboard", plan.screens[0].id);
    TEST_ASSERT_EQUAL_UINT8(2, plan.screens[0].tile_count);
    TEST_ASSERT_EQUAL_UINT8(2, plan.screens[0].tiles[0].col_span);
}

static void test_unsupported_layout_type_rejected() {
    auto cfg = parse_json(R"({
        "widgets": {"items": {"sog": {"type": "numeric"}}},
        "layout": {"screens": [{"id":"x", "type":"absolute", "tiles":[]}]}
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_FALSE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParseCode::UnsupportedLayoutType),
                          static_cast<int>(err.code));
}

static void test_variant_matching_picks_compatible() {
    auto cfg = parse_json(R"({
        "widgets": {"items": {"a": {"type": "numeric"}}},
        "layout": {
            "variants": [
                {"id": "wide", "match": {"display": {"width": 800, "height": 480}}, "screens": [{"id":"x","type":"grid","tiles":[]}]},
                {"id": "sq",   "match": {"display": {"width": 480, "height": 480}}, "screens": [{"id":"y","type":"grid","tiles":[]}]}
            ]
        }
    })");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_TRUE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_STRING("sq", plan.layout_variant);
    TEST_ASSERT_EQUAL_UINT8(1, plan.screen_count);
    TEST_ASSERT_EQUAL_STRING("y", plan.screens[0].id);
}

static void test_too_many_widgets_rejected() {
    // Construct a payload with 33 widgets (over MAX_WIDGETS=32).
    std::string body = "{\"widgets\":{\"items\":{";
    for (int i = 0; i < 33; ++i) {
        if (i) body += ",";
        body += "\"w";
        body += std::to_string(i);
        body += "\":{\"type\":\"numeric\"}";
    }
    body += "}}}";
    auto cfg = parse_json(body.c_str());
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_FALSE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParseCode::TooManyWidgets),
                          static_cast<int>(err.code));
}

static void test_empty_config_is_valid_empty_plan() {
    auto cfg = parse_json("{}");
    RenderPlan plan;
    ParseError err;
    TEST_ASSERT_TRUE(parse(cfg, 480, 480, plan, err));
    TEST_ASSERT_EQUAL_UINT8(0, plan.widget_count);
    TEST_ASSERT_EQUAL_UINT8(0, plan.screen_count);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_widget_type_round_trip);
    RUN_TEST(test_minimal_widgets_parses);
    RUN_TEST(test_unsupported_widget_type_rejected);
    RUN_TEST(test_display_mismatch_rejected);
    RUN_TEST(test_screen_references_missing_widget_rejected);
    RUN_TEST(test_screen_with_grid_layout_accepted);
    RUN_TEST(test_unsupported_layout_type_rejected);
    RUN_TEST(test_variant_matching_picks_compatible);
    RUN_TEST(test_too_many_widgets_rejected);
    RUN_TEST(test_empty_config_is_valid_empty_plan);
    return UNITY_END();
}
