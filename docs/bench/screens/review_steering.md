# Steering screen review

QuadGrid: HDG/CTS compass (TL), XTE (TR), VMG (BL), RUDDER (BR), plus a
[-10][-1][+1][+10] course-button row at the bottom.

## Displayed values (5 questions each)

### HDG/CTS compass tile (TL)
- HDG `037` (compass), CTS `321` (yellow secondary), N/E/S/W cardinal ring,
  red + green wind/marker pointers.
1. Plausible? OK — HDG 037 vs live 39.5deg; CTS 321 vs live ~321. Both match.
2. Unit correct? OK — degrees implied on a compass; no glyph needed.
3. Precision/scaling? OK — 3-digit zero-padded heading is conventional.
4. Label clear? OK — "HDG / CTS" header; "CTS 321" labeled.
5. Legible? OK — large cyan numerals on the dial, good contrast, no clip.
VERDICT: OK

### XTE tile (TR)
- `68.07kS` (scaled metres, k suffix, S = starboard side).
1. Plausible? OK in the sense it reflects the absurd sim XTE (-268 km);
   the scaling math is right, the value is just sim-degenerate.
2. Unit correct? OK — scaled metres with k; S side suffix is the design.
3. Precision/scaling? OK — quantize to k is the intended new behavior.
4. Label clear? OK — "XTE".
5. Legible? PROBLEM — the value OVERFLOWS the tile. The string renders
   `68.07kS` but the final `S` is clipped at the right tile edge (reads
   `68.07k>` cut off mid-glyph) and the leading digit nearly touches the
   left edge. Font is far too large for a 7-char scaled string in this
   tile width. This is the flagged overflow and it is real.
VERDICT: PROBLEM (overflow / clipped suffix)

### VMG tile (BL)
- `-3.7 kn`.
1. Plausible? OK — live VMG -3.77 kn; -3.7 matches.
2. Unit correct? OK — kn.
3. Precision/scaling? OK — 1 decimal.
4. Label clear? OK — "VMG".
5. Legible? OK — fits, good contrast.
VERDICT: OK

### RUDDER tile (BR)
- `3P deg` (magnitude + P/S helm side).
1. Plausible? OK — live rudder -2.3deg (port); rounds to 3deg port = `3P`.
2. Unit correct? OK — deg.
3. Precision/scaling? MINOR — integer rounding of 2.3 -> 3 is a bit coarse
   but acceptable for a helm tile.
4. Label clear? OK — "RUDDER"; P=port reads fine.
5. Legible? OK — fits well.
VERDICT: OK

### Course-button row
- `[-10] [-1] [+1] [+10]` along the bottom.
1/2/3/4. N/A (controls). Labels clear, symmetric.
5. Legible? OK — buttons readable, but they sit very low and slightly
   crowd the VMG/RUDDER tiles' bottom padding; acceptable.
VERDICT: OK

## Problems
- XTE tile overflow (HIGH): value `68.07kS` is too wide; trailing `S`
  side-suffix is clipped at the right tile edge and the first digit hugs
  the left edge. The big-font autosize does not account for the k-scaled
  string + side suffix. Fix: shrink font / fit-to-width for the XTE
  number, or reserve the suffix in a smaller secondary glyph. Even with a
  sane (non-sim) XTE like "0.27kS" the suffix placement is at risk.
- Course buttons sit low/tight against the bottom tiles (LOW).
