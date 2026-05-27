"""Touch + gesture injection via /api/test/* endpoints.

Exercises BOTH injection levels:

  Level 1 - touchscreen manager
    /api/test/touch   raw press/release into the touch snapshot
    /api/test/tap     synthesised press+hold+release
    /api/test/swipe   intermediate samples + drive detect_swipe_release

  Level 2 - action queue
    /api/test/gesture post directly into app::Command{ShowScreen, ...}

The high-level gesture path is the most reliable assertion (it bypasses
calibration / hit testing entirely), so the priority test uses it.
The low-level swipe path is also asserted to prove the full pipeline.
"""
import time

import pytest


def _ensure_dashboard(device):
    device.show_screen("dashboard")
    device.wait_for_screen("dashboard")


def test_gesture_left_goes_next(device):
    """Level 2: post 'left' -> next screen."""
    _ensure_dashboard(device)
    before = device.state()["screen"]["index"]
    r = device.gesture("left")
    assert r["ok"], r
    time.sleep(0.6)
    after = device.state()["screen"]["index"]
    assert after != before, f"screen didn't advance: {before} -> {after}"


def test_gesture_right_goes_prev(device):
    """Level 2: post 'right' returns to dashboard from screen 1."""
    _ensure_dashboard(device)
    device.gesture("left")
    device.wait_for_screen_change_from("dashboard") if hasattr(
        device, "wait_for_screen_change_from") else time.sleep(0.6)
    device.gesture("right")
    time.sleep(0.6)
    s = device.state()["screen"]["id"]
    # 'right' on screen[1] takes us back to dashboard (or the wrap-around
    # behaviour, depending on the carousel). Either way it should change.
    assert s == "dashboard"


def test_gesture_up_opens_settings(device):
    _ensure_dashboard(device)
    r = device.gesture("up")
    assert r["ok"], r
    device.wait_for_screen("settings", timeout_s=4)


def test_gesture_down_returns_to_dashboard(device):
    device.show_screen("settings")
    device.wait_for_screen("settings")
    r = device.gesture("down")
    assert r["ok"], r
    device.wait_for_screen("dashboard", timeout_s=4)


def test_gesture_unknown_rejected(device):
    r = device.gesture("diagonal")
    assert r["ok"] is False


# --- Level 1: low-level synthesised swipe ----------------------------

def test_swipe_left_navigates(device):
    """Inject a 300 ms left swipe across the middle of the screen.
    The high-level detector sees dx negative + above SWIPE_MIN_PX
    and posts 'next'.
    """
    _ensure_dashboard(device)
    before = device.state()["screen"]["index"]
    r = device.swipe(x0=400, y0=240, x1=80, y1=240, dur_ms=300)
    assert r["ok"], r
    time.sleep(0.8)
    after = device.state()["screen"]["index"]
    assert after != before, f"swipe-left didn't advance ({before} -> {after})"


def test_swipe_up_opens_settings(device):
    _ensure_dashboard(device)
    r = device.swipe(x0=240, y0=420, x1=240, y1=80, dur_ms=300)
    assert r["ok"], r
    device.wait_for_screen("settings", timeout_s=4)


def test_swipe_too_short_does_nothing(device):
    """Below SWIPE_MIN_PX = 80 the detector should reject."""
    device.show_screen("settings")
    device.wait_for_screen("settings")
    r = device.swipe(x0=240, y0=240, x1=240, y1=240 - 30, dur_ms=200)
    assert r["ok"], r
    time.sleep(0.6)
    # Stayed on settings (no swipe-down detected).
    assert device.state()["screen"]["id"] == "settings"


# --- Level 1: raw tap into the touch snapshot ------------------------

def test_tap_on_dashboard_tile(device):
    """Inject a tap inside the top-left WIND tile (approx 120, 180 on
    a 480x480 quad_grid). The tile_clicked_cb should fire and post
    ShowScreen("wind").

    Coordinates are POST-calibration; the test endpoint writes them
    straight into the calibrated snapshot. So this assertion is robust
    to any calibration matrix.
    """
    _ensure_dashboard(device)
    r = device.tap(x=120, y=180, hold_ms=80)
    assert r["ok"], r
    device.wait_for_screen("wind", timeout_s=4)


def test_tap_outside_any_tile_is_inert(device):
    """A tap in the MOB-safe band (y < 62) shouldn't trigger a tile."""
    _ensure_dashboard(device)
    before = device.state()["screen"]["id"]
    device.tap(x=10, y=10, hold_ms=80)
    time.sleep(0.6)
    assert device.state()["screen"]["id"] == before


def test_raw_touch_press_release(device):
    """Lowest-level injection - drive g_touch directly. We don't assert
    a UI side-effect here (LVGL hit-test might or might not produce a
    CLICKED depending on motion / hold). We just verify the endpoint
    returns ok and the state echo matches.
    """
    r = device.touch(x=240, y=240, pressed=True)
    assert r["ok"] and r["pressed"] is True and r["x"] == 240
    r = device.touch(x=0, y=0, pressed=False)
    assert r["ok"] and r["pressed"] is False
