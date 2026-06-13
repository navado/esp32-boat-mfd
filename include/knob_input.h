#pragma once

// Encoder + push-button input source for the Waveshare knob. Runs a
// dedicated task that classifies quadrature detents (ESP32Encoder/PCNT)
// and button gestures (OneButton) into knob::Event values, posted to the
// UI queue as app::CommandType::Knob. Device-only; empty on other boards.

namespace knob_input {

// Start the encoder/button task. No-op unless BOARD_ID_WAVESHARE_KNOB_1_8.
void setup();

}  // namespace knob_input
