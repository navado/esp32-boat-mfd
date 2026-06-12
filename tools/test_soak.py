import unittest
from soak_verdict import analyze, format_verdict, Sample


class TestVerdict(unittest.TestCase):
    def test_detects_reboot_on_uptime_regression(self):
        samples = [
            Sample(t=0,  uptime_ms=600000, heap=120000, sk_live=True),
            Sample(t=30, uptime_ms=630000, heap=119000, sk_live=True),
            Sample(t=60, uptime_ms=5000,   heap=121000, sk_live=True),  # reboot
        ]
        v = analyze(samples)
        self.assertEqual(v.reboots, 1)
        self.assertFalse(v.passed)

    def test_unresponsive_poll_is_not_a_reboot(self):
        # uptime_ms=0 means the device didn't answer; must NOT count as reboot.
        samples = [
            Sample(t=0,  uptime_ms=600000, heap=120000, sk_live=True),
            Sample(t=30, uptime_ms=0,      heap=0,      sk_live=False),
            Sample(t=60, uptime_ms=660000, heap=119000, sk_live=True),
        ]
        v = analyze(samples)
        self.assertEqual(v.reboots, 0)
        self.assertEqual(v.usable, 2)

    def test_detects_stall_via_explicit_age(self):
        samples = [
            Sample(t=0,  uptime_ms=1000,  heap=120000, sk_live=True, sk_age_ms=200),
            Sample(t=30, uptime_ms=31000, heap=120000, sk_live=True, sk_age_ms=35000),
        ]
        v = analyze(samples, stall_ms=30000)
        self.assertEqual(v.stalls, 1)
        self.assertFalse(v.passed)

    def test_detects_stall_via_liveness_flag_when_no_age(self):
        samples = [
            Sample(t=0,  uptime_ms=1000,  heap=120000, sk_live=True),
            Sample(t=30, uptime_ms=31000, heap=120000, sk_live=False),  # not live
        ]
        v = analyze(samples)
        self.assertEqual(v.stalls, 1)
        self.assertFalse(v.passed)

    def test_clean_run_passes(self):
        samples = [
            Sample(t=i * 30, uptime_ms=1000 + i * 30000, heap=120000 - i * 10,
                   sk_live=True)
            for i in range(10)
        ]
        v = analyze(samples)
        self.assertEqual(v.reboots, 0)
        self.assertEqual(v.stalls, 0)
        self.assertTrue(v.passed)
        self.assertEqual(v.min_heap, 120000 - 9 * 10)

    def test_all_unresponsive_fails(self):
        samples = [Sample(t=i * 30, uptime_ms=0, heap=0, sk_live=False)
                   for i in range(5)]
        v = analyze(samples)
        self.assertEqual(v.usable, 0)
        self.assertFalse(v.passed)

    def test_format_verdict_renders(self):
        v = analyze([Sample(t=0, uptime_ms=1000, heap=120000, sk_live=True)])
        out = format_verdict(v)
        self.assertIn("verdict", out)
        self.assertIn("PASS", out)


if __name__ == "__main__":
    unittest.main()
