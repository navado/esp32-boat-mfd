# Wind screen review

Fullscreen 360° wind rose + four numeric tiles (AWS / AWA / TWS / TWA).

## Displayed values (5 questions each)

### Center HDG — "040°"
1. Plausible? Live HDG = 39.5°. Rounds to 040°. OK.
2. Unit correct? OK — degree symbol present.
3. Precision/scaling? OK — zero-padded 3-digit heading (040), conventional for marine.
4. Label clear? OK — "HDG" label above.
5. Legible? OK — large blue digits centered, no clipping.

### Bezel cardinal labels (N/E/S/W + NE/SE/SW/NW)
1. Plausible? Rose is rotated to heading; N near top-ish, cardinals placed around rim. OK.
2. Unit correct? N/A (labels).
3. Precision/scaling? N/A.
4. Label clear? OK — cardinals legible.
5. Legible? PROBLEM: cardinal labels are NOT kept upright. "W" on the left, "S" lower-right, "NE" at top are rotated/tilted with the bezel. Spec calls for cardinals upright (N/E/S/W readable). They follow bezel rotation here.

### AWS tile — "6.3" + "kn"
1. Plausible? Live AWS = 6.0. 6.3 close. OK.
2. Unit correct? OK — "kn".
3. Precision/scaling? OK — one decimal.
4. Label clear? OK — "AWS".
5. Legible? OK — orange digits, no clipping.

### AWA tile — "134P"
1. Plausible? Live AWA = -127.5° (port). 134P = 134° port. Magnitude close, side correct (port=P). OK.
2. Unit correct? PROBLEM: no degree symbol; "P" suffix denotes port but the value is an angle in degrees with no ° unit. Unit header row shows "kn" only over AWS/TWS; AWA/TWA have no unit at all in the header.
3. Precision/scaling? OK — integer degrees.
4. Label clear? OK — "AWA", P=port suffix is a sensible convention.
5. Legible? OK — white digits, no clipping.

### TWS tile — "9.8" + "kn"
1. Plausible? Live TWS = 9.3. 9.8 close. OK.
2. Unit correct? OK — "kn".
3. Precision/scaling? OK — one decimal.
4. Label clear? OK — "TWS".
5. Legible? OK.

### TWA tile — "153P"
1. Plausible? Live TWA = -149.4° (port). 153P close, side correct. OK.
2. Unit correct? PROBLEM: same as AWA — no ° unit shown for the angle tiles.
3. Precision/scaling? OK — integer degrees.
4. Label clear? OK — "TWA", P suffix.
5. Legible? OK.

### Wind markers on rim (apparent = blue arrow into center; true = orange; small glyphs)
1. Plausible? AWA -127.5° (port) and TWA -149.4° (port) → both wind from the port quarter (aft-port). The blue apparent arrow and orange marker sit in the lower-left/left (port) region — correct side. OK.
2. Unit correct? N/A.
3. Precision/scaling? N/A.
4. Label clear? MINOR — markers unlabeled; relies on color convention (blue=apparent, orange=true).
5. Legible? OK — distinct, on correct (port) side.

### Center small number — "0.8"
1. Plausible? Unclear what this is (likely a boat-speed or VMG overlay near the boat icon). Not in spec list. Cannot verify.
2. Unit correct? PROBLEM: no unit/label — "0.8" floating by the boat icon is unexplained.
3-5. Ambiguous element; flag for clarification.

## Problems
1. Cardinal bezel labels are rotated with the bezel, not kept upright — spec violation (N/E/S/W should be readable). Most severe.
2. AWA / TWA tiles show no degree unit (only P/S suffix); unit header row carries "kn" for AWS/TWS but nothing for the angle tiles.
3. Unexplained "0.8" near boat icon — no label/unit; meaning unclear.
4. Wind markers unlabeled (color-only convention) — minor.
