#include "knob_input.h"

#if defined(BOARD_ID_WAVESHARE_KNOB_1_8)

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <OneButton.h>

#include "app_events.h"
#include "board_pins.h"
#include "knob_menu.h"

namespace knob_input {

namespace {

ESP32Encoder s_enc;
OneButton s_btn(ENC_BTN, /*activeLow=*/true, /*pullupActive=*/true);
int64_t s_last_count = 0;

void post_event(knob::Event ev, bool held = false) {
    app::Command c;
    c.type = app::CommandType::Knob;
    c.i = (int32_t)ev;
    c.b[0] = held ? '1' : '0';
    c.b[1] = '\0';
    c.t_post_us = micros();
    app::post(c, 0);
}

void on_click() {
    post_event(knob::Event::Click);
}
void on_double() {
    post_event(knob::Event::DoubleClick);
}
void on_long_start() {
    post_event(knob::Event::LongPress);
}

void knob_task(void *) {
    // PCNT counts 4 transitions per detent on full-quadrature; divide so one
    // physical detent = one event. Adjust divisor after bench check.
    constexpr int kCountsPerDetent = 4;
    for (;;) {
        s_btn.tick();
        int64_t count = s_enc.getCount();
        int64_t delta = count - s_last_count;
        // Read button pin at the moment of detent emission — this is the only
        // point where the held flag matters. ENC_BTN is active-low with pull-up:
        // pressed == LOW. Avoids the stale-flag bug where a short click left
        // s_held=true via OneButton callbacks (longPressStop never fired).
        bool held = (digitalRead(ENC_BTN) == LOW);
        while (delta >= kCountsPerDetent) {
            post_event(knob::Event::DetentCW, held);
            s_last_count += kCountsPerDetent;
            delta -= kCountsPerDetent;
        }
        while (delta <= -kCountsPerDetent) {
            post_event(knob::Event::DetentCCW, held);
            s_last_count -= kCountsPerDetent;
            delta += kCountsPerDetent;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

}  // namespace

void setup() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    s_enc.attachFullQuad(ENC_A, ENC_B);
    s_enc.clearCount();
    s_last_count = 0;

    // ENC_BTN is read directly in knob_task at detent time for the held flag;
    // ensure the pin is configured as input with pull-up for reliable reads.
    pinMode(ENC_BTN, INPUT_PULLUP);

    s_btn.attachClick(on_click);
    s_btn.attachDoubleClick(on_double);
    s_btn.attachLongPressStart(on_long_start);
    s_btn.setPressMs(400);  // long-press threshold

    xTaskCreatePinnedToCore(knob_task, "knob", 4096, nullptr, 2, nullptr, 0);
}

}  // namespace knob_input

#else  // not the knob board

namespace knob_input {
void setup() {
}
}  // namespace knob_input

#endif
