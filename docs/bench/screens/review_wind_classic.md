# Wind classic screen review

Fullscreen 360° wind rose with large in-dial hero readouts.

## Displayed values (5 questions each)

### Center HDG — "036°"
1. Plausible? Live HDG = 39.5°. 036° is a bit low vs 39.5 but within wander. OK (borderline).
2. Unit correct? OK — degree symbol.
3. Precision/scaling? OK — zero-padded 3-digit.
4. Label clear? OK — "HDG" above.
5. Legible? OK — large blue digits, centered top.

### AWS hero — "8.0" + "124P" below
1. Plausible? Live AWS = 6.0. 8.0 is higher than expected (wander, but ~33% off). MINOR — plausible-ish but on the high side. "124P" angle close to AWA 127.5° port. OK.
2. Unit correct? PROBLEM: AWS "8.0" has no "kn" unit inside the dial; the "124P" angle below has no ° unit. Hero readouts drop units.
3. Precision/scaling? OK — one decimal for speed, integer for angle.
4. Label clear? OK — "AWS" header; the 124P underneath implies AWA but isn't separately labeled.
5. Legible? OK — orange, no overlap with the central boat icon.

### TWS hero — "11.7" + "145P" below
1. Plausible? Live TWS = 9.3. 11.7 is ~25% high. MINOR — on the high side but plausible with wander. "145P" close to TWA 149.4° port. OK.
2. Unit correct? PROBLEM: no "kn" on 11.7, no ° on 145P.
3. Precision/scaling? OK.
4. Label clear? OK — "TWS" header; 145P implies TWA, unlabeled.
5. Legible? OK — white, no overlap.

### SOG tile (bottom-left) — "5.1" + "kn"
1. Plausible? Live SOG = 4.29. 5.1 is ~19% high. MINOR — somewhat high vs truth but plausible.
2. Unit correct? OK — "kn".
3. Precision/scaling? OK — one decimal.
4. Label clear? OK — "SOG".
5. Legible? OK — green, no clipping.

### SOW tile (bottom-right) — "5.2" + "kn"
1. Plausible? Live STW (speed-thru-water) = 4.39. 5.2 is ~18% high. MINOR — high but plausible.
2. Unit correct? OK — "kn".
3. Precision/scaling? OK.
4. Label clear? PROBLEM: labeled "SOW" — non-standard. The water-speed abbreviation is STW (Speed Through Water); "SOW" is a typo/wrong abbreviation. Should be STW.
5. Legible? OK — cyan, no clipping.

### Bezel labels — cardinals (N/E/S/W/NE/SE/SW/NW) + tick numbers (30/60/90/120/150)
1. Plausible? Tick numbers represent degrees off-axis on each side; arrangement consistent with a rotating rose. OK.
2. Unit correct? N/A (ring scale).
3. Precision/scaling? OK — 30° increments.
4. Label clear? OK.
5. Legible? PROBLEM: cardinal labels are NOT kept upright — "W" and others tilt with the bezel rotation. Spec expects cardinals readable/upright.

### Center small number — "0.8"
1. Plausible? Same unexplained "0.8" as the wind screen, by the boat icon. Cannot verify.
2. Unit correct? PROBLEM: no label/unit.
3-5. Ambiguous overlay element.

### Wind markers (orange true / blue apparent arrows on rim)
1. Plausible? Both winds from port (AWA/TWA negative). Apparent blue arrow points into center from upper area; orange true marker lower-left (port). Side broadly correct. OK.
2-5. Unlabeled color convention; legible, no overlap.

## Problems
1. "SOW" label is wrong abbreviation for speed-through-water — should be "STW". Most severe (incorrect label).
2. Cardinal bezel labels rotate with the bezel instead of staying upright — spec violation.
3. In-dial hero readouts (AWS/TWS speeds and their angle sub-values) carry no units (no "kn", no °).
4. AWS 8.0 / TWS 11.7 read noticeably high vs live truth (6.0 / 9.3) — within wander but worth confirming not a stale/scale issue.
5. Unexplained "0.8" near boat icon — no label/unit.
