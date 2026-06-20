# Autopilot screen review

Hand-built compass + marker ring (top), HDG number + COG/SOG sub-line, an
XTE PORT/STBD strip, four tiles (DEPTH/SPEED/AWS/AWA), header with
STBY / AUTO / HOME.

## Displayed values (5 questions each)

### Header
- `STBY` (left button), `AUTO` (center, green), `HOME` (right button).
1. Plausible? OK — AUTO mode active (green), STBY toggle, HOME nav.
2. Unit correct? N/A.
3. Precision? N/A.
4. Label clear? OK.
5. Legible? OK — good contrast, buttons fit.
VERDICT: OK

### Compass + marker ring
- HDG arc with cardinal N/E + tick labels (30/60/120/330), red/yellow/green
  markers near the top (HDG/COG/CTS/AP-target), a red index pointer lower-left.
1. Plausible? OK — markers cluster near top (HDG 38, COG 42, CTS 321-ish
   wraps) consistent with live; AP-target marker present (the cluster of
   colored glyphs at top).
2. Unit correct? OK — degrees on dial.
3. Precision? OK.
4. Label clear? MINOR — multiple overlapping marker glyphs at the top are
   hard to disambiguate (which is COG vs AP-target vs CTS); a legend would
   help but it's a dense-by-design dial.
5. Legible? OK — dial on white face, dark ticks, readable.
VERDICT: OK (marker cluster slightly ambiguous)

### HDG number + sub-line
- `38.0°`, sub-line `COG 042° | SOG 3.7 kn`.
1. Plausible? OK — HDG 38.0 vs live 39.5; COG 042 vs live 43.3; SOG 3.7 vs
   live 4.29 (a bit low but values wander). All consistent.
2. Unit correct? OK — deg glyph present, kn present.
3. Precision? OK — HDG 1 decimal, COG zero-padded int, SOG 1 decimal.
4. Label clear? OK — COG/SOG labeled inline.
5. Legible? OK — large dark numerals on white dial; sub-line gray on dark,
   readable.
VERDICT: OK

### XTE PORT/STBD strip
- Scale `-1.0 ... 0 ... 1.0`, PORT (left) / STBD (right) labels, a red
  marker pinned near the far-left (~ -1.0).
1. Plausible? OK — live XTE is hugely negative (port); marker pinned to the
   port rail is the correct saturation behavior.
2. Unit correct? OK — relative deflection bar, no unit needed; numeric
   scale shown.
3. Precision? OK.
4. Label clear? OK — PORT/STBD + numeric scale.
5. Legible? OK — though the marker is right at the rail with no "saturated/
   off-scale" cue; could show a clamp arrow. Minor.
VERDICT: OK

### Tile: DEPTH
- `2667.7 m`.
1. Plausible? OK relative to sim — live depth-below-keel 2670 m; 2667.7
   matches (absurd depth is the sim, not the firmware).
2. Unit correct? OK — m.
3. Precision? MINOR — 1 decimal on a 2667 m depth is false precision, and
   this is NOT k-scaled here while XTE/DTW elsewhere ARE scaled — an
   inconsistency. A 2667.7 m depth could be "2.67k" for consistency.
4. Label clear? OK — "DEPTH", unit "m" in header.
5. Legible? PROBLEM(minor) — `2667.7` nearly fills the tile width and the
   leading digit sits at the left edge; close to clipping but appears to
   just fit. Watch for overflow on a wider value.
VERDICT: OK (precision/scaling inconsistency vs other screens)

### Tile: SPEED
- `3.8 kn`.
1. Plausible? OK — STW live 4.39 / SOG 4.29; 3.8 a bit low but wanders.
2. Unit correct? OK — kn.
3. Precision? OK.
4. Label clear? OK.
5. Legible? OK.
VERDICT: OK

### Tile: AWS
- `5.3 kn` (yellow/amber).
1. Plausible? OK — live AWS 6.0; 5.3 close.
2. Unit correct? OK — kn.
3. Precision? OK.
4. Label clear? OK.
5. Legible? OK — amber color draws attention (wind highlight), good contrast.
VERDICT: OK

### Tile: AWA
- `134P` (magnitude + Port side).
1. Plausible? OK — live AWA -127.5deg port; 134 port close (wanders).
2. Unit correct? MINOR — no degree glyph; `134P` relies on context.
3. Precision? OK — integer.
4. Label clear? OK — "AWA"; P = port.
5. Legible? OK.
VERDICT: OK

## Problems
- DEPTH precision/scaling inconsistency (MEDIUM): shows `2667.7 m` with a
  false 0.1 m decimal and is NOT k-scaled, while XTE/DTW on other screens
  ARE k-scaled. Pick one convention; depth this large should drop the
  decimal (and arguably scale).
- DEPTH value nearly fills tile width — leading digit at the left edge,
  near clipping; verify fit on larger values (LOW).
- Compass top marker cluster (HDG/COG/CTS/AP-target) overlaps and is hard
  to disambiguate (LOW); consider distinct shapes/legend.
- XTE strip marker saturates at the rail with no off-scale clamp cue (LOW).
- AWA/RUDDER-style tiles lack a degree glyph (LOW).
