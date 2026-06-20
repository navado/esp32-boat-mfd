# Screen Visual QA — Run 1 → Run 2 Comparison

Two-pass visual QA of all 11 device screens. Run 1 = pre-fix; Run 2 = post-fix
(commit 907233d: `fit_value_font`, depth `BELOW KEEL` label, wind_classic
`SOW`→`STW`, autopilot DEPTH k-scaler). Sources: `RUN1_REPORT.md`, `RUN2_REPORT.md`.

> Caveat: the Run-2 reviewer evaluated two render variants per fixed screen — a
> "top-level" render and a "run2" variant. For depth/steering/route/autopilot the
> fix is sometimes present only in the cited "run2" variant while the top-level
> render still shows the pre-fix output. Verifications below note where the two
> disagree.

## Verdict deltas

| Screen | Run1 verdict | Run2 verdict | Run1 #probs | Run2 #probs | Net change |
|---|---|---|---|---|---|
| dashboard | MINOR | MINOR | 3 | 4 | +1 (no fix targeted; markers split into its own item) |
| wind | PROBLEM | MINOR | 4 | 5 | improved verdict; cardinal-rotation cleared, stray `0.8` remains |
| wind_classic | PROBLEM | PROBLEM | 6 | 7 | +1 (STW relabel done; cardinal rotation now flagged here) |
| wind_steer | PROBLEM | PROBLEM | 5 | 5 | unchanged (XTE strip still numberless / now reported absent) |
| nav | MINOR | MINOR | 6 | 4 | −2 (consolidated; same core issues) |
| depth | PROBLEM | PROBLEM | 6 | 5 | −1 (BELOW-KEEL label still missing; units still absent) |
| steering | PROBLEM | PROBLEM | 5 | 3 | −2 (consolidated; XTE overflow persists in run2 variant) |
| route | PROBLEM | PROBLEM | 5 | 5 | unchanged (DTW left-clip fixed in run2 variant; XTE still broken) |
| autopilot | MINOR | MINOR | 4 | 5 | +1 (DEPTH k-scale only in run2 variant; top render still `2667.7`) |
| trip | MINOR | MINOR | 5 | 4 | −1 (consolidated; same issues) |
| status | MINOR | MINOR | 4 | 4 | unchanged |

Aggregate: PROBLEM screens 6 → 5 (wind recovered to MINOR). Problem-count deltas
are mostly re-grouping noise; the substantive movement is wind's verdict upgrade
and the partial XTE/DTW improvements visible only in the run2 render variants.

## Fix verification

### 1. `fit_value_font()` — auto-shrink hero font so wide scaled values (XTE/DTW) fit QuadGrid tiles
Targeted Run-1 problems:
- steering #1 — "XTE tile clips value on both horizontal edges; auto-fit fails on the wide k-scaled string".
- route #1/#2/#3 — XTE/DTW value overflow, leading digits clipped off the left edge, values not constrained to the tile.

Run-2 evidence:
- route — **PARTIAL.** The run2 variant fixed the *horizontal value clipping* on DTW and XTE ("run2 values fit within their tiles"; DTW `452.73 nm` and VMG fit). But the *top-level* route render still shows `453.95 nm` clipping its leading digit and `nm`, and XTE still renders `263.21kS m` overrunning the right edge (wrong magnitude/side aside). DTW left-clip: resolved in run2 variant. XTE width: not resolved.
- steering — **NOT RESOLVED.** Run2 variant of the same layout still renders `263.21kS m` "colliding with the right border"; the top render shows `68.07kS` clipped mid-`S` with the `m` unit pushed off-tile. fit_value_font did not shrink the degenerate k-scaled XTE string enough to fit.

Verdict: **PARTIAL** — fixes the general DTW/normal-width left-clip (route run2 variant) but the worst-case k-scaled XTE string still overflows on both steering and route.

### 2. Depth HeroPlus now shows the hero binding label "BELOW KEEL"
Targeted Run-1 problem:
- depth #1 — "HERO not labeled as below-keel; bare `2.7k` is ambiguous".

Run-2 evidence:
- depth HERO element: shown `2.7k m`, note "carries no 'below keel' sublabel — only the screen title 'Depth' disambiguates it"; combined problem #1 still reads "HERO tile is NOT labeled as below-keel … it has no label at all".

Verdict: **NOT RESOLVED** — Run 2 shows no `BELOW KEEL` label on the depth hero; the problem persists verbatim.

### 3. wind_classic mislabel "SOW" → "STW"
Targeted Run-1 problem:
- wind_classic #5 — "STW shown as nonstandard `SOW` in a corner box".

Run-2 evidence:
- wind_classic in-dial hero element: shown "… SOG 5.1, SOW/STW 5.2 in corner pills"; the unit element note references "SOG/STW corner tiles". The corner pill now reads **STW** (no standalone `SOW` problem item remains in Run-2's problem list).

Verdict: **RESOLVED** — corner readout relabeled STW; the `SOW` mislabel no longer appears as a problem. (COG still absent — a separate, pre-existing item.)

### 4. Autopilot DEPTH tile now uses the k/M scaler (was over-precise "2667.7 m")
Targeted Run-1 problem:
- autopilot #1 — "DEPTH tile bypasses k-scaling — shows `2667.7 m` over-precise and edge-clipped instead of `2.7k m`".

Run-2 evidence:
- autopilot DEPTH element: shown `DEPTH m 2667.7`, note "over-precise and NOT k-scaled as expected (should be `2.7k`)". Combined: "top-level screen shows `2667.7` (raw) while run2 shows `2.7k` (k-scaled)". So the k-scaler works in the run2 render variant but the top-level render still emits `2667.7`.

Verdict: **PARTIAL** — k-scaling confirmed in the run2 variant (`2.7k`), but the top-level autopilot render still shows raw `2667.7 m`; the over-precision/overflow is not eliminated in the primary capture.

Summary: RESOLVED ×1 (wind_classic STW), PARTIAL ×2 (fit_value_font, autopilot DEPTH),
NOT-RESOLVED ×1 (depth BELOW KEEL).

## Still open

Problems present in BOTH runs (regressions left untouched or only partially fixed), grouped by theme.

### Cardinal labels rotating with the bezel
- wind_classic — labels N/E/S/W rotate with the bezel instead of staying upright (Run-1 implicitly OK per-element, Run-2 combined flags it as problem #1 — a spec violation in both passes; wind_classic was the screen where Run-1 said cardinals were upright, so this is the most worrying open item). Still open.
- wind — per-element confirms cardinals stay upright (the good case); no regression there.

### Stray / unlabeled center "0.8" boat-speed readout
- wind #2 (Run-1) → wind #1/#2 (Run-2): center value `0.8`/`0.0`, unitless, unlabeled, implausible vs ~9 kn wind and SOG ~4.3 kn. Still open (now the top problem on wind).
- wind_classic: the small `0.8` STW-ish readout floats over the boat icon / AWA arrow, tiny and unlabeled — present in both runs (Run-1 #2, Run-2 #3). Still open.

### Units omitted on angle / sub-tiles
- depth: TEMP no `C`, SOG no `kn`, TWA no degree — Run-1 #2, Run-2 #2. Still open.
- wind / wind_classic / wind_steer / autopilot / nav: AWA/TWA (and HDG/COG) angle tiles omit the degree symbol and rely on bare P/S suffixes — recurring inconsistency in both runs. Still open.

### Missing-unit on the WIND dial (dashboard)
- dashboard: WIND round-dial value shows a bare number with no `kn` while SOG/DEPTH show units — Run-1 #1, Run-2 #1. Still open.

### XTE strip shows no numeric value
- wind_steer: XTE strip is a bar-only gauge with no numeric cross-track value (Run-1 #2; Run-2 #1 — Run-2 reports it as absent entirely). Still open.
- autopilot: XTE strip provides no numeric magnitude and doesn't reflect the degenerate live value (Run-1 #4, Run-2 #3). Still open.

### Degenerate sim XTE / route value (wrong magnitude & side)
- steering / route: live XTE −267769 m (−268 km) renders as a clipped/garbled `kS` token with wrong magnitude (263.21k vs ~268k, stale frame) and wrong side (`S`=starboard for a port/negative value). Run-1 (steering #3, route #1/#5) and Run-2 (steering #3, route #1/#5). The display has no clamp/elide guard for absurd XTE magnitudes. Still open — this is a data/format problem distinct from the font-fit fix.

### Marker-ring bug clusters at the top of the dial
- wind_steer (#3→#4) and autopilot (#3→#4): HDG/COG/CTS/AP-target glyphs collapse into an indistinguishable cluster when bearings converge, overlapping the `30` tick label. Still open in both runs.

### Missing CTS field on route
- route #4 (both runs): CTS course-to-steer not rendered; route shows only 4 of the expected fields, no progress bar. Still open.

### nav HDG digits obscure W/E cardinals
- nav #1 (both runs): the large `040` heading number overlaps and hides the W and E cardinal letters. Still open.

### Duplicated COG on nav
- nav #2 (both runs): COG shown twice (orange sublabel in HDG tile + dedicated COG tile). Still open.

### Wasted space themes (low severity, both runs)
- depth hero empty lower band; trip DISTANCE hero ~45% whitespace; status right-half dead band; dashboard numeric tiles under-use vertical space. All still open.

## New in Run 2

Problems that appear in Run-2 but not Run-1 — watching specifically for
`fit_value_font` side-effects (uneven/smaller value fonts).

- **No font side-effects observed.** Run 2 does not report any value rendering
  smaller-than-siblings, uneven hero scaling, or a shrunk-but-misaligned value as
  a consequence of `fit_value_font`. Where fonts were noted (DTW/XTE), the
  remaining issue is *still-overflowing* width, not an over-shrunk font — i.e. the
  fit logic under-shrinks rather than over-shrinks.
- wind_classic cardinal-label rotation surfaced as an explicit **problem #1** in
  Run 2, whereas Run 1 marked wind_classic cardinals as upright/OK. This is either
  a genuine regression or a stricter Run-2 read; flagged as the screen's top
  problem now and worth a targeted re-check on hardware.
- autopilot picked up a normalized **"XTE strip doesn't reflect the live −268 km
  value / no off-scale indication"** framing (#3) and a **per-tile decimal-precision
  not normalized** item (#5) that were not separately itemized in Run-1's
  autopilot list. These are descriptive refinements of existing behavior rather
  than new defects.
- steering added a **course-adjust row clipped against the bottom frame edge**
  observation (element-level `clipped`) that Run-1 described only as "too close to
  bottom edge" — borderline new, low severity.

No fit_value_font regressions. The net-new items are a possible wind_classic
cardinal-rotation regression and a handful of finer-grained restatements of
pre-existing autopilot/steering issues.
