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
volatile bool s_held = false;
int64_t s_last_count = 0;

void post_event(knob::Event ev) {
    app::Command c;
    c.type = app::CommandType::Knob;
    c.i = (int32_t)ev;
    c.b[0] = s_held ? '1' : '0';
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
// attachPress fires on the initial depress — set held so a
// detent emitted while the button is down carries held=true.
void on_press_start() {
    s_held = true;
}
// attachLongPressStop fires when button is released after a long hold.
// Use it (rather than a short-click release) to clear held so the
// release of a hold-and-rotate also clears the flag.
void on_release() {
    s_held = false;
}

void knob_task(void *) {
    // PCNT counts 4 transitions per detent on full-quadrature; divide so one
    // physical detent = one event. Adjust divisor after bench check.
    constexpr int kCountsPerDetent = 4;
    for (;;) {
        s_btn.tick();
        int64_t count = s_enc.getCount();
        int64_t delta = count - s_last_count;
        while (delta >= kCountsPerDetent) {
            post_event(knob::Event::DetentCW);
            s_last_count += kCountsPerDetent;
            delta -= kCountsPerDetent;
        }
        while (delta <= -kCountsPerDetent) {
            post_event(knob::Event::DetentCCW);
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

    s_btn.attachClick(on_click);
    s_btn.attachDoubleClick(on_double);
    s_btn.attachLongPressStart(on_long_start);
    // attachPress fires immediately on depress (used to set the held flag
    // so hold-and-rotate detents arrive with b[0]='1').
    s_btn.attachPress(on_press_start);
    s_btn.attachLongPressStop(on_release);
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
