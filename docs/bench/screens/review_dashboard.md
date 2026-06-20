# Dashboard screen review

2x2 overview grid: WIND / SOG / DEPTH / BATT.

## Displayed values (5 questions each)

### WIND tile — round dial, value "5.7" (no unit), orange ring, small port/stbd tick markers
1. Plausible? Live AWS = 6.0 kn. 5.7 is close (values wander), so OK as a slightly-stale AWS reading.
2. Unit correct? PROBLEM: no unit shown at all. Every other tile has a unit; AWS should read "kn". A bare "5.7" inside a wind dial is ambiguous (could be read as AWA/TWS).
3. Precision/scaling? OK — one decimal for a speed is reasonable.
4. Label clear? PARTIAL: label is "WIND", but the number is AWS specifically. "WIND" is fine for a tile title; the missing unit is the real gap.
5. Legible? OK — large orange digits, good contrast, no clipping. The two small tick glyphs (orange/green) at lower-left of the ring are tiny and their meaning is unclear, but not clipped.

### SOG tile — "4.3" + "kn"
1. Plausible? Live SOG = 4.29 kn. Matches. OK.
2. Unit correct? OK — "kn".
3. Precision/scaling? OK — one decimal.
4. Label clear? OK — "SOG".
5. Legible? OK — large blue digits, unit subscripted cleanly, no clipping.

### DEPTH tile — "2.7k" + "m"
1. Plausible? Live depth-below-keel = 2670 m. 2670 -> "2.7k" is correct. OK (the value itself is the absurd sim depth, but the render is faithful).
2. Unit correct? OK — "m" retained per the new scaling rule (label stays "m", magnitude scales to k).
3. Precision/scaling? OK — k-suffix scaling working as designed (2670 -> 2.7k).
4. Label clear? OK — "DEPTH".
5. Legible? OK — no clipping; "k" and subscript "m" both fit.

### BATT tile — "86%" + green bar (~90% filled)
1. Plausible? No live battery truth provided; 86% is plausible. OK.
2. Unit correct? OK — "%".
3. Precision/scaling? OK — integer percent.
4. Label clear? OK — "BATT".
5. Legible? MINOR: bar fill (~90%) visually disagrees with the "86%" text; the fill looks fuller than 86%. Possibly a rounding/track-inset artifact. Digits legible, no clipping.

## Problems
1. WIND tile shows no unit ("5.7" bare) while all sibling tiles show units — inconsistent; should read "kn" (or be labeled AWS). Most severe.
2. BATT bar fill appears ~90% but text says 86% — bar/value mismatch (cosmetic).
3. WIND dial's two small tick glyphs (orange/green) are tiny and unlabeled — unclear meaning, but not broken.
