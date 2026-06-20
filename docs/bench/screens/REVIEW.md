# On-device screen QA — 2026-06-20

Captured all 11 screens live from the device (10.42.0.67) with live SignalK
data, reviewed each against its intended design via subagents (one detail file
per screen: `review_<screen>.md`, with 5 evaluative questions per displayed
value). Live ground truth at capture: HDG 39.5°, COG 43.3°, SOG 4.29 kn,
STW 4.39 kn, AWA −127.5°(P), AWS 6.0 kn, TWA −149.4°(P), TWS 9.3 kn,
depth-below-keel 2670 m, water temp 18.5 °C, rudder −2.3°(P), VMG −3.77 kn,
**XTE −267 769 m (−268 km — degenerate sim route)**, CTS ~321°.

## Verdicts

| Screen | Verdict | Headline problems |
|---|---|---|
| dashboard | MINOR | WIND tile has no unit (kn); BATT bar fill (~90%) ≠ text (86%) |
| wind | PROBLEMS | Cardinal labels rotate WITH bezel (should stay upright); AWA/TWA tiles show no ° unit; stray unlabeled "0.8" by boat icon |
| wind_classic | PROBLEMS | "SOW" should be "STW"; cardinals rotate with bezel; in-dial hero readouts drop all units; stray "0.8" |
| wind_steer | MINOR | XTE strip shows P/S ends but **no numeric value**; course row [-10…+10][TACK][GYBE] placed cleanly (no overlap) ✓ |
| nav | MINOR | SOG 3.5 vs 4.29 kn (smoothing lag); else clean, position well formatted |
| depth | PROBLEMS | Hero "2.7k m" **unlabeled** (doesn't say "below keel"); TEMP/SOG subtiles have no units; TWA = degraded "true-wind compass" (template gap) |
| steering | PROBLEMS | **XTE "68.07kS" overflows the tile** — leading digit + trailing S clipped (k-scale autosize ignores the suffix); RUDDER 3P° ✓, VMG ✓ |
| route | PROBLEMS | DTW "453.95" clipped left + unit shows "**nn**" (not "nm") clipped right; XTE "68.06kS" same overflow; no progress bar |
| autopilot | MINOR | DEPTH "2667.7 m" — **not k-scaled + false 0.1 m precision** (hand-built screen bypasses the formatter); near-clips tile |
| trip | MINOR | NOW 4.1 vs 4.3 kn (smoothing); distance/time/avg cross-check OK |
| status | MINOR | Internal HEAP 22 kB free (low — SRAM-pressure watch); link fields all live ✓ |

## Recurring themes (root causes)

1. **k-scaled value + P/S suffix overflows QuadGrid tiles** (steering/route XTE, route DTW). The big hero font + "NN.NNk" + side-suffix is too wide; the autosize doesn't account for the suffix. *Introduced/exacerbated by the value-scaling change.* Fix: shrink-to-fit font, or a smaller suffix glyph, or drop the suffix when scaled.
2. **Units omitted** on angle tiles (AWA/TWA °), wind in-dial hero (kn/°), and depth subtiles (°C/kn). Inconsistent with SOG/DEPTH which show units.
3. **Cardinal labels rotate with the bezel** on both wind roses (should stay upright). Pre-existing, not from this round's changes.
4. **Hand-built screens bypass the new formatter** → autopilot DEPTH isn't k-scaled and shows false precision, inconsistent with templated screens. (Known limitation flagged earlier.)
5. **Depth hero unlabeled** (below-keel vs below-transducer ambiguous) — from the depth restructure.
6. **DTW unit "nn"** (route) — likely a unit-string typo to verify.
7. Sim-side: **XTE −268 km** is a degenerate route in the simulator (not a firmware bug, but it surfaces the overflow).

## Wins confirmed on-device
- Value scaling live: depth 2670 m → "2.7k m", DTW/XTE k-scaled.
- RUDDER tile renders on steering (3P°), tracking the now-realistic sim rudder.
- VMG displays (−3.7 kn) — subscription fix works.
- Course-adjust buttons present on steering + wind_steer (TACK/GYBE), no overlap.
- HDG live and moving (sim wander fix); link "live", values match ground truth.
