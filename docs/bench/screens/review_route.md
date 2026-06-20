# Route screen review

Route progress quad: DTW (TL), BTW (TR), XTE (BL), VMG (BR).

## Displayed values (5 questions each)

### DTW tile (TL)
- `453.95 nn` (distance to waypoint).
1. Plausible? OK-ish — a DTW value from the route; magnitude unverified
   but not absurd.
2. Unit correct? PROBLEM — unit renders as `nn`, not `nm`. Either a typo
   or the `m` is clipped so only `nn` shows (the spec says scaled nm). The
   unit glyph is also half-cut at the right edge.
3. Precision/scaling? MINOR — 2 decimals (453.95) is a lot of precision
   for a distance-to-waypoint; the spec expects scaled nm (k/M), so a raw
   453.95 suggests scaling may not be applied here, or it's already nm.
4. Label clear? OK — "DTW".
5. Legible? PROBLEM — the number `453.95` is left-clipped (leading `4`
   touches/loses the left edge) and the `nn`/`nm` unit is clipped at the
   right edge. Font too large for the tile.
VERDICT: PROBLEM

### BTW tile (TR)
- `339` (bearing to waypoint, degrees).
1. Plausible? OK — bearing 339deg; CTS is ~321, BTW 339 is plausible for a
   route leg.
2. Unit correct? MINOR — no degree glyph shown; a `deg`/`°` would help.
3. Precision/scaling? OK — integer bearing.
4. Label clear? OK — "BTW".
5. Legible? OK — fits, good contrast.
VERDICT: OK (add degree unit)

### XTE tile (BL)
- `68.06kS` (scaled metres, k, S side).
1. Plausible? OK — reflects the same absurd sim XTE (-268 km); scaling
   math correct, value sim-degenerate. Note it differs slightly from the
   steering XTE (68.07 vs 68.06) — values wander, fine.
2. Unit correct? OK — scaled m with k + S side.
3. Precision/scaling? OK.
4. Label clear? OK — "XTE".
5. Legible? PROBLEM — same overflow as the steering XTE: leading `6` is
   clipped at the left tile edge and the trailing `S` suffix is cut off at
   the right edge. The scaled-string + suffix is too wide for the tile.
VERDICT: PROBLEM (overflow / clipped both ends)

### VMG tile (BR)
- `-3.8 kn`.
1. Plausible? OK — live VMG -3.77 kn; -3.8 matches.
2. Unit correct? OK — kn.
3. Precision/scaling? OK — 1 decimal.
4. Label clear? OK — "VMG".
5. Legible? OK — fits.
VERDICT: OK

## Problems
- DTW tile overflow + unit (HIGH): `453.95` clipped at the left edge, unit
  reads `nn` and is clipped at the right edge (should be `nm`). Fit-to-width
  needed; verify the unit string is "nm" not "nn".
- XTE tile overflow (HIGH): same defect as steering — k-scaled value plus
  P/S suffix is wider than the tile; both leading digit and trailing side
  suffix are clipped.
- BTW has no degree unit (LOW).
- No progress bar present despite the spec mentioning "maybe a progress
  bar" — not shown; flag only if intended.
