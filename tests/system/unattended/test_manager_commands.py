"""Lane B - command poll + ack (spec 17 F4 / spec 18 S5)."""
import os
import time

import pytest

ENABLED = os.environ.get("ESPDISP_MANAGER_CONTRACT") == "1"
pytestmark = pytest.mark.skipif(
    not ENABLED, reason="ESPDISP_MANAGER_CONTRACT=1 not set")


def _register(device, manager):
    device.post_cmd(f"manager-register {manager.base_url}")
    deadline = time.time() + 10
    while time.time() < deadline:
        if manager.devices:
            return next(iter(manager.devices.values()))
        time.sleep(0.5)
    pytest.fail("device never registered")


def _wait_ack(manager, dev, cid, timeout_s=20):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        cmd = next((c for c in dev.commands if c.id == cid), None)
        if cmd and cmd.state in ("acknowledged", "failed"):
            return cmd
        time.sleep(0.5)
    pytest.fail(f"command {cid} never acked")


def test_screen_set_command(device, manager):
    dev = _register(device, manager)
    device.show_screen("dashboard")
    cid = manager.queue_command(dev.id, "screen.set", {"id": "wind"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"
    assert device.state()["screen"]["id"] == "wind"


def test_brightness_set_command(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "brightness.set", {"value": 32})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"
    # SetBrightness ack fires when manager queues the app::Command;
    # actual apply happens on the LVGL task. Allow a short window
    # for the drain.
    deadline = time.time() + 5
    last = None
    while time.time() < deadline:
        last = device.state()["display"]["brightness"]
        if last == 32:
            return
        time.sleep(0.5)
    pytest.fail(f"brightness never reached 32 (last={last})")


def test_beep_command(device, manager):
    """beep is a no-op on boards without a beeper but must still ack ok."""
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "beep", {"duration_ms": 50})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"


def test_unknown_command_is_unsupported(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "definitely.not.a.thing", {})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "unsupported_command"


def test_audit_log_records_command_lifecycle(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "screen.set", {"id": "depth"})
    _wait_ack(manager, dev, cid)
    events = [e for e in manager.audit if e.get("cid") == cid]
    types = {e["event"] for e in events}
    assert {"command.created", "command.ack"}.issubset(types)


# ---- spec 17 §8 v1 commands added later ----------------------------------

def test_overlay_show_then_clear(device, manager):
    """overlay.show pins the alarm banner with the operator message;
    overlay.clear releases it. Both must ack ok."""
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "overlay.show",
                                {"message": "TESTING"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok", cmd.ack_result
    cid2 = manager.queue_command(dev.id, "overlay.clear", {})
    cmd2 = _wait_ack(manager, dev, cid2)
    assert cmd2.ack_result == "ok", cmd2.ack_result


def test_overlay_show_accepts_text_fallback(device, manager):
    """Plugins that use `text` instead of `message` must still work."""
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "overlay.show",
                                {"text": "FROM-TEXT-KEY"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"


def test_overlay_show_empty_payload_rejected(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "overlay.show", {})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "invalid_payload"


def test_log_level_string_payload(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "log.level", {"level": "warn"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok", cmd.ack_result
    # Restore default so subsequent tests still see info-level logs.
    cid2 = manager.queue_command(dev.id, "log.level", {"level": "info"})
    _wait_ack(manager, dev, cid2)


def test_log_level_numeric_payload(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "log.level", {"level": 3})  # ESP_LOG_INFO
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"


def test_log_level_unknown_string_rejected(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "log.level", {"level": "spammy"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "invalid_payload"


def test_log_level_out_of_range_rejected(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "log.level", {"level": 99})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "invalid_payload"


def test_touch_mode_poll_always_accepted(device, manager):
    """touch.mode "poll" must succeed regardless of board wiring -
    detaches the INT line if any, falls back to the polling timer."""
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "touch.mode", {"mode": "poll"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "ok"


def test_touch_mode_unknown_value_rejected(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "touch.mode", {"mode": "vibration"})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "invalid_payload"


def test_touch_mode_missing_payload_rejected(device, manager):
    dev = _register(device, manager)
    cid = manager.queue_command(dev.id, "touch.mode", {})
    cmd = _wait_ack(manager, dev, cid)
    assert cmd.ack_result == "invalid_payload"
