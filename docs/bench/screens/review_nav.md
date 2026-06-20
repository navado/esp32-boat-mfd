# Visual QA — nav.png

Resolution 480x480. Ground truth: HDG 39.5°, COG 43.3°, SOG 4.29 kn.
2x2 tile layout: HDG (with mini compass), SOG, COG, POS.

## Displayed values (5 questions each)

### HDG tile — "040" + mini compass, sub-line "COG 045"
- (1) Ground truth HDG 39.5° → rounds to 040. OK
- (2) Degrees (implicit, no glyph). Acceptable for heading. OK
- (3) Zero-padded 3-digit integer (040) — correct nautical convention. OK
- (4) "HDG" label top-left; sub-line "COG 045" (orange) clearly labeled. OK
- (5) Mini compass with N/E/S/W cardinals + red/green vessel markers renders
     cleanly; number legible over it. OK

### SOG tile — "3.5 kn"
- (1) Ground truth SOG 4.29 kn; shown 3.5. Off by ~0.8 kn — more than expected
     wander. MINOR (could be timing, but notable).
- (2) "kn". OK
- (3) One decimal. OK
- (4) "SOG". OK
- (5) Large, legible. OK

### COG tile — "045"
- (1) Ground truth COG 43.3° → 043; shown 045. Within wander. OK
- (2) Degrees implicit. OK
- (3) Zero-padded 3-digit integer. OK
- (4) "COG". OK
- (5) Legible. OK

### POS tile — "37°53.310'N  017°44.960'E"
- (1) Plausible lat/lon (mid-latitude, near sim region). OK
- (2) Degrees/decimal-minutes with N/E hemisphere — correct marine format. OK
- (3) 3-decimal minutes, lon zero-padded to 3 deg digits. OK
- (4) "POS" label. OK
- (5) Two lines, fits within tile, no clipping. OK

## Problems
1. SOG reads 3.5 kn vs ground-truth 4.29 kn (~0.8 kn low). Units/precision fine;
   value lower than wander would explain. Worth a glance at SOG source/timing.
2. No other issues — formatting, padding, hemisphere labels, and layout are all
   correct; no clipping or overlap.

Overall: clean screen. Only flag is the SOG value gap.
