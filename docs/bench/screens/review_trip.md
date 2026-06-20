# Trip screen review (trip.png)

Ground truth: SOG 4.29 kn, STW 4.39 kn, heading 39.5°, moving boat, link live.

## Displayed values (5 questions each)

### DISTANCE — 203.0 nm
1. Plausible: OK — a cumulative trip/odometer of 203.0 nm is plausible for an underway boat.
2. Unit/format: OK — "nm" (nautical miles), correct for marine distance.
3. Precision/scaling: OK — one decimal. With the new distance->k/M scaling, 203 stays sub-k so it renders raw; fine. (Worth confirming >999 nm scales to "k" not garbage, but not testable here.)
4. Label: OK — "DISTANCE" clear.
5. Legible: OK — large hero numeral, "nm" unit set off to the right, no clipping. Lots of empty card space but no overlap.

### TIME UNDERWAY — 35:07:51
1. Plausible: OK — 35h 07m 51s elapsed; consistent with a trip distance of 203 nm at a few knots (203/35 ≈ 5.8 kn avg, which matches AVG SPEED below — internally consistent).
2. Unit/format: OK — HH:MM:SS. Note hours exceed 24 and roll into a 2-digit hour field (35), so format is cumulative-hours, not clock — correct intent.
3. Precision/scaling: OK.
4. Label: OK — "TIME UNDERWAY" clear.
5. Legible: OK — fits the tile, no truncation.

### AVG SPEED — 5.8 kn
1. Plausible: OK — 203.0 nm / 35.13 h = 5.78 kn ≈ 5.8 kn. Consistent with distance+time. Slightly above current NOW (4.1) which is fine for an average.
2. Unit/format: OK — "kn".
3. Precision/scaling: OK — one decimal.
4. Label: OK.
5. Legible: OK.

### MAX SPEED — 8.1 kn
1. Plausible: OK — peak 8.1 kn ≥ avg 5.8 and ≥ now 4.1; ordering correct.
2. Unit/format: OK — "kn".
3. Precision/scaling: OK.
4. Label: OK.
5. Legible: OK — rendered in accent green, readable.

### NOW — 4.1 kn
1. Plausible: OK — current speed 4.1 kn. Ground truth SOG 4.29 / STW 4.39. 4.1 is close; likely SOG rounded down or a slightly older sample. Within tolerance.
2. Unit/format: OK — "kn".
3. Precision/scaling: MINOR — 4.1 vs SOG 4.29 rounds to 4.3 and STW 4.39 rounds to 4.4. 4.1 is ~0.2 kn low. Could be lag/smoothing or sourcing a different speed field; not a hard error but slightly off the live truth.
4. Label: OK — "NOW" clear (current speed).
5. Legible: OK — centered, accent blue, no clipping.

## Problems
- NOW shows 4.1 kn while live SOG=4.29 / STW=4.39 (rounds to 4.3/4.4). ~0.2 kn discrepancy — likely smoothing/lag or wrong speed field; minor.
- DISTANCE card has large empty area (cosmetic only); value/unit placement fine.
- No data is stale/zero; all fields live and internally consistent (distance/time/avg cross-check). No clipping/overflow.

Verdict: MINOR
