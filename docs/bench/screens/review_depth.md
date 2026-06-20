# Visual QA — depth.png

Resolution 480x480. Ground truth: depth-below-keel 2670 m, water temp 18.5°C,
SOG 4.29 kn, TWA -149.4°P. Restructured: hero (depth-below-keel) + 3 subtiles
(TEMP, SOG, TWA-numeric-instead-of-compass).

## Displayed values (5 questions each)

### Hero — "2.7k" + unit "m"
- (1) Ground truth depth-below-keel 2670 m → scaled "2.7k". OK (value correct).
- (2) Unit "m". OK
- (3) Scaling correct (2670 → 2.7k). One sig-fig after the k. OK
- (4) PROBLEM — the hero has NO label. The screen title is "Depth" but the hero
     is specifically depth-BELOW-KEEL, and that distinction (vs below-transducer
     2672 m) is not shown anywhere. A user can't tell which reference. PROBLEM.
- (5) Big cyan number, legible. BUT the unit "m" is detached far to the right of
     "2.7k" with a wide gap, looking disconnected/orphaned rather than a unit
     suffix. MINOR layout.

### Subtile — TEMP "18.5"
- (1) Ground truth water temp 18.5°C — exact match. OK
- (2) PROBLEM — no unit shown (no °C). MINOR-to-PROBLEM (ambiguous; 18.5 could
     be misread).
- (3) One decimal. OK
- (4) "TEMP" label. OK
- (5) Legible. OK

### Subtile — SOG "4.4"
- (1) Ground truth SOG 4.29 kn → 4.4 (rounds up; plausible wander). OK
- (2) PROBLEM — no unit (no "kn"). MINOR.
- (3) One decimal. OK
- (4) "SOG". OK
- (5) Legible. OK

### Subtile — TWA "144P"
- (1) Ground truth TWA -149.4°P → magnitude 149; shown 144P. Within wander, sign
     (Port) correct. OK
- (2) P suffix (degrees implicit). OK
- (3) Integer. OK
- (4) "TWA" label. OK
- (5) Legible. OK
- NOTE (known gap): this tile was intended to be a TRUE-WIND COMPASS widget but
  the template can't host a compass, so it degraded to a numeric TWA readout.
  Documented design gap — flagged here as intended-but-unrealized.

## Problems
1. Hero is unlabeled — does not state "below keel" (or distinguish from
   below-transducer 2672 m). The only label is the generic screen title "Depth".
   A depth display that hides its reference datum is a real usability gap.
2. Known gap: TWA subtile is a numeric fallback for what was meant to be a
   true-wind compass widget (template can't host a compass). Carried as-is.
3. Subtiles TEMP and SOG show no units (°C, kn). The hero shows "m" but subtiles
   are bare numbers — inconsistent and mildly ambiguous.
4. Hero unit "m" sits with a large gap to the right of "2.7k", reading as
   detached rather than a unit suffix. (layout/cosmetic)

Overall: hero value + scaling correct, but missing the "below keel" label is the
main problem; unit-less subtiles and the known compass-to-numeric degradation
follow.
