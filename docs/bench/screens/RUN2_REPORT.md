# Screen Visual QA — Run 2 (post-fix)

Per-element + combined visual QA review of all 11 device screens against expected
element lists, after commit 907233d (fit_value_font, depth BELOW KEEL label,
wind_classic SOW→STW, autopilot DEPTH k-scaling).

## Summary

| Screen | Verdict | # problems | Top problem |
|---|---|---|---|
| dashboard | MINOR | 4 | WIND dial omits its `kn` unit while every other tile shows one |
| wind | MINOR | 5 | Center boat-speed reads near-zero `0.8`/`0.0` despite ~9 kn TWS (broken/zero feed) |
| wind_classic | PROBLEM | 7 | Cardinal labels N/E/S/W rotate with the bezel instead of staying upright |
| wind_steer | PROBLEM | 5 | XTE strip shows no numeric cross-track value |
| nav | MINOR | 4 | Large `040` HDG digits overlap/obscure the W and E cardinal letters |
| depth | PROBLEM | 5 | HERO still carries no `below-keel` label (only screen title disambiguates) |
| steering | PROBLEM | 3 | XTE value overflows/clips at right edge; `kS` token ambiguous |
| route | PROBLEM | 5 | XTE renders `263.21kS m` — wrong magnitude, wrong side, run-on token |
| autopilot | MINOR | 5 | DEPTH tile shows raw over-precise `2667.7 m` (top render not k-scaled) |
| trip | MINOR | 4 | DISTANCE hero box wastes ~half its area as empty space |
| status | MINOR | 4 | Large dead horizontal band on the right for the 11 non-bar rows |

---

### dashboard — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| WIND round dial tile (AWS + kn) | 6.2 | yes | good | ok | MINOR | Orange value plausible vs 6.0 kn + green dir arrow, but `kn` unit label missing inside dial. |
| SOG tile (value + kn) | 4.3 kn | yes | good | ok | OK | Large cyan value with unit, matches SOG ~4.3 kn, no clipping. |
| DEPTH tile (value + m, k/M scaled) | 2.7k m | yes | good | ok | OK | Correctly k-scales 2670 m; legible, clear label, no clipping. |
| BATT tile (percent + fill bar) | 86% + ~85% green bar | yes | good | ok | OK | Large blue `86%` with proportional green fill, well-placed. |

Combined:
- Overlaps: WIND tile's two dir caret glyphs (orange + green) sit lower-left, touching/crowding the dial ring stroke (visual collision, not hard text overlap); no tile-boundary overlaps, 2x2 gutters respected.
- Inconsistencies: WIND dial shows bare `6.2` with NO unit while SOG `kn`/DEPTH `m` do; mixed value metaphors (one dial, two numerics, one numeric+bar); WIND value orange while SOG/DEPTH/BATT blue (reads as alert); labels consistent grey uppercase top-left.
- spaceUtilization: WIND tile well-filled by dial; SOG/DEPTH/BATT value-only with large empty lower/right regions; big numbers sit high-left, not vertically centered — numeric tiles waste vertical space.
- clarity: All four boxes delineated and legible; WIND least clear (missing unit + small low-contrast markers); DEPTH `2.7k`+`m` k-scaling reads correctly.
- problems:
  1. WIND dial omits its `kn` unit while every other tile shows a unit — inconsistent/ambiguous.
  2. WIND dir-marker carets crowd the lower-left dial ring and overlap the stroke.
  3. Numeric tiles (SOG/DEPTH/BATT) under-use vertical space; values sit high leaving empty lower regions.
  4. WIND value orange vs blue siblings — lone warm color can be misread as an alert.
- whatWorks: Clean uniform 2x2 grid, consistent gutters/captions; values plausible (SOG 4.3 exact, DEPTH 2.7k, WIND 5.7~6.0, BATT 86%); high-contrast dark theme; DEPTH k-scaling with retained `m`; BATT fill bar gives proportional read.

---

### wind — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| rotating compass bezel + ticks | ring rotated to HDG 034° at top index | yes | good | ok | OK | Even ticks + cardinals, centered, correctly aligned to ~040° index. |
| cardinal labels (MUST stay upright) | N/E/S/W (+ NE/SE/SW/NW); E & W verified upright | yes | good | ok | OK | Each glyph renders upright with good contrast, no overlap. |
| center HDG number | 040° | yes | good | ok | OK | Large cyan, matches HDG ~39.5°, no clipping. |
| apparent-wind marker | orange `A` + needle at W rim (~273°) | yes | good | ok | OK | Consistent with AWA −127° port over HDG 040°; distinct from `T`. |
| true-wind marker | cyan `T` near SW rim | yes | good | ok | OK | Consistent with TWA ~−149° port on HDG-up dial; not clipped. |
| AWS bottom tile (kn) | AWS 6.3 kn | yes | good | ok | OK | Orange, labeled, plausible vs 6.0 kn. |
| AWA bottom tile (deg P/S) | 134P | yes | good | ok | OK | Encodes port side, plausible vs −127°. |
| TWS bottom tile (kn) | 9.8 kn | yes | good | ok | OK | Labeled, plausible vs 9.3 kn. |
| TWA bottom tile (deg P/S) | TWA 134P | yes | good | ok | OK | Port-suffixed, plausible magnitude, no clipping. |

Combined:
- Overlaps: no hard overlaps in bottom tile row; dial rim congested in lower-left quadrant (orange/blue wind arrows crowd NW/W/SW labels); red/green heading-reference arc floats just under HDG label / near NE.
- Inconsistencies: center boat-speed `0.8`/`0.0` grossly inconsistent with 9 kn TWS and SOG ~4.3 kn (implausible near-zero); center value has NO unit/caption; intercardinals same weight as cardinals; AWA/TWA use `P` suffix without degree while HDG uses `040°`; ≥4 unlabeled rim markers (yellow diamond, red chevron, orange/blue arrows) distinguished only by color, no legend.
- spaceUtilization: dial dominates upper ~75% but interior under-used (empty bands left/right of center stack); bottom tile row compact/efficient; round-bezel corners dead black.
- clarity: bottom tiles highly legible; HDG `040°` clear; weak: center speed small/low-contrast/unlabeled, rim markers ambiguous, top arc unlabeled.
- problems:
  1. Center boat-speed reads near-zero (`0.8`/`0.0`) despite ~9 kn wind and SOG/STW ~4.3 — wrong binding/unit or stale/zero feed; most serious (headline metric looks broken).
  2. Center speed value has no unit/caption (unlike every other number).
  3. Rim wind markers (apparent vs true) unlabeled, color-only; need A/T or legend.
  4. Lower-left dial quadrant congested (markers + speed + W/SW/NW labels compete).
  5. Angle-format inconsistency: HDG `°` vs AWA/TWA bare + P/S.
- whatWorks: bottom AWS/AWA/TWS/TWA strip strongest (consistent, color-accented, unit-labeled, plausible vs truth); cardinal labels stay UPRIGHT as specified; boat-icon + heading-arc metaphor clean; green rim ring strong frame.

---

### wind_classic — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| rotating bezel + ticks | cardinals + 30/60/90/120/150 + radial ticks | yes | good | ok | OK | Renders cleanly around full dial, oriented to HDG 037, no clipping. |
| cardinal labels (MUST stay upright) | N/E/S/W | yes | good | ok | OK | (per-element) all four upright, good contrast — but combined flags rotation (see below). |
| in-dial hero readouts | HDG 036°, AWS 8.0, TWS 11.7, AWA 124P, TWA 145P; SOG 5.1, STW 5.2 (corner pills); COG absent | yes | good | ok | MINOR | Heroes legible/plausible; SOG/STW corner pills now labeled STW (was SOW); COG missing. |
| wind markers on rim | tick glyphs + colored apex markers (yellow diamond, orange tri, white flag) | yes | good | ok | OK | Distinct, well-placed on ring; per-element no overlap (combined flags label collisions). |
| unit labels on hero readouts | `kn` on SOG/STW, `°` on HDG; AWS/TWS none | yes | good | ok | OK | Present/legible; AWS/TWS omit `kn` (design choice). |

Combined:
- Overlaps: lower-left orange-triangle marker overlaps rotated `W` label and `120` tick; upper-left yellow rim glyph crowds `N` and the red arc start; `0.8` STW-ish readout sits unlabeled between boat icon and AWS column, colliding visually with boat + blue AWA arrow; white check glyph sits on the `150` tick label.
- Inconsistencies: cardinal labels rotate WITH the bezel rather than staying upright (spec violation); units inconsistent (SOG/STW show `kn`, in-dial AWS/TWS/HDG no `kn`; HDG `°` but AWS/TWS none); AWA/TWA bare `124P`/`145P` no label/degree; hero colors mixed (AWS orange, TWS white, HDG cyan, SOG green, STW cyan) with no legend; `0.8` tiny vs large AWS/TWS heroes.
- spaceUtilization: dial interior crowded in upper-center band (HDG + AWS/TWS columns + boat + AWA arrow stacked y≈90–300) while lower half empty; SOG/STW corner boxes fill otherwise-dead corners; COG/depth/temp/rudder/VMG/XTE/CTS absent leaving lower dial underused.
- clarity: HDG/AWS/TWS heroes + SOG/STW boxes legible; AWA/TWA smaller but readable; `0.8` too small/unlabeled; rotated cardinals legible but disorienting.
- problems:
  1. Cardinal labels N/E/S/W rotate with the bezel, not upright — explicit spec violation.
  2. AWS/TWS hero readouts have no `kn` unit, inconsistent with SOG/STW boxes.
  3. The `0.8` readout is tiny, unlabeled, and collides with the boat icon / AWA arrow.
  4. Wind marker rim glyphs overlap cardinal labels and tick numbers (W, 120, 150).
  5. AWA/TWA (124P/145P) lack any unit/degree or AWA/TWA label; rely on unexplained P/S.
  6. Lower half of dial wasted while upper-center crowded; COG/depth/temp/rudder/VMG/CTS not surfaced.
  7. No legend for hero color coding (orange/white/cyan/green).
- whatWorks: big-three hero stack (HDG center, AWS left, TWS right) strong/scannable; red→green bezel arc + blue AWA needle give at-a-glance wind sense; SOG/STW fill dead corners with explicit `kn`; values plausible vs truth (AWA 124P/TWA 145P, SOG/STW order-of-magnitude, AWS/TWS ballpark).

---

### wind_steer — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| compass dial + no-go/target sectors | HDG 36.2° dial, green target + red no-go, top caret | yes | good | ok | OK | Renders cleanly within frame, no clipping/overlap; plausible vs ~39.5°. |
| marker ring (HDG/COG/CTS/TWD bugs) | red chevron top, cyan HDG bug ~30°, red CTS ~330° + ticks | yes | good | ok | OK | (per-element) distinct bugs; combined flags top cluster overlap. |
| center HDG + TWA sub-line | HDG 36.2°, "TWA 139°P \| TWD 257°" | yes | good | ok | OK | Centered, large, legible; plausible vs HDG ~39.5 / TWA −149P. |
| XTE strip (should show numeric value) | (none) | no | unreadable | ok | PROBLEM | No XTE/cross-track element present; tiles show AWS/AWA/TWS/TWA only, no numeric XTE. |
| AWS/AWA/TWS/TWA wind tiles | AWS 12.4 kn, AWA 42S, TWS 14.5 kn, TWA 46S | yes | good | ok | OK | Even bottom row, labeled, `kn` on speeds, high-contrast, no clipping. |
| ON/STBY button | STBY | yes | good | ok | OK | Top-left rounded button, clear white-on-dark. |
| HOME button | HOME | yes | good | ok | OK | Top-right corner, legible, not clipped. |
| course-adjust row | -10 -1 +1 +10 | yes | good | ok | OK | Crisp well-spaced pills along bottom. |
| TACK button | TACK | yes | good | ok | OK | Between +10 and GYBE, legible. |
| GYBE button | GYBE | yes | good | ok | OK | Bottom-right of action row, no overlap. |

Combined:
- Overlaps: XTE strip PORT/STBD/scale labels overlap tops of AWS/TWA tiles (strip crushed against tile row ~y310–345); XTE center `0` collides with TWS tile header (renders `0 TWS`); marker-ring bugs cluster 30–60° (green target/red HDG-COG chevron/cyan smear; green triangle obscures `30` tick); PORT-end saturated tick overlaps `PORT` text.
- Inconsistencies: AWS/TWS carry `kn`, AWA/TWA use P/S no unit (intentional but mixes styles); AWS amber while others white with no alarm reason; XTE strip is the only purely-graphical gauge (numeric −1.0..1.0 scale but NO actual value); ON/STBY split across `STBY` button + separate green `AUTO` title.
- spaceUtilization: dial fills top ~60%; XTE strip squeezed into a thin sliver fighting the tile row (no breathing room); bottom action row well spaced; semicircular corners hold normal empty arc bg. Net: no large dead areas, but XTE/tile band crowded.
- clarity: most boxes legible (HDG, TWA/TWD sub-line, four tiles, buttons); breaks on (1) XTE strip — `0 TWS` collision + missing number make it uninterpretable, (2) marker bug cluster (HDG vs COG vs target indistinguishable).
- problems:
  1. XTE strip shows no numeric value; for degenerate −267769 m it pegs PORT correctly but magnitude unreadable.
  2. XTE `0` center label overlaps TWS tile header (`0 TWS`) — hard collision.
  3. XTE strip vertically compressed; PORT/STBD labels overlap tile tops — needs separation.
  4. Marker-ring bugs overlap into unreadable cluster 30–60° (`30` tick partly hidden).
  5. Autopilot affordance ambiguous: `STBY` button + separate `AUTO` title, no single ON/STBY toggle.
- whatWorks: semicircular dial with green target/red no-go reads well as focal point; large center HDG + TWA/TWD sub-line crisp; four uniform aligned wind tiles; even full-width bottom action row; values plausible vs truth.

---

### nav — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| position lat/lon (DDM) | 37°55.280'N / 017°47.129'E | yes | good | ok | OK | Correct DDM in cyan, well-placed, no clipping. |
| COG value | 045 | yes | good | ok | OK | Large cyan, matches ~43°, no overlap. |
| SOG value | 3.5 kn | yes | good | ok | OK | Large cyan + unit, plausible vs ~4.3 kn. |
| HDG value | 040 | yes | good | ok | OK | In compass dial, matches ~39.5°. |
| mini compass/heading indicator | 040 center, N/S/W marks, "COG 045" sublabel | yes | good | ok | OK | Circular dial w/ cardinal ring + sub-label, legible. |

Combined:
- Overlaps: HDG tile large `040` digits overlap the compass ring and collide with `W`/`E` cardinal letters (W largely obscured); red/cyan top markers crowd `N` and the tile top edge / `HDG` title; no tile-to-tile overlaps.
- Inconsistencies: SOG shows `kn` but HDG/COG bare numbers no degree; COG shown twice (orange `COG 045` sublabel in HDG tile + standalone cyan COG tile); precision differs (SOG 3.5 vs zero-padded 040/045); in-compass COG sublabel orange vs cyan COG tile.
- spaceUtilization: even 2x2 grid edge-to-edge; SOG/COG carry large empty space around one number; HDG tile densest/borderline crowded; POS uses two-line space well; no large dead zones.
- clarity: each tile legible (large cyan on dark); weak spot HDG compass — W/E low-contrast and hidden behind big digits; POS crisp DDM; SOG `kn` small but readable.
- problems:
  1. HDG compass: large digits overlap/obscure W and E cardinal letters.
  2. COG duplicated (orange sublabel + dedicated tile) — redundant.
  3. Missing degree units on HDG/COG while SOG carries `kn`.
  4. Compass markers crowd the top edge and `N` label.
- whatWorks: clean balanced 2x2 grid, consistent styling/gutters; all five expected elements present; big-number typography reads at distance; POS correct DDM with hemisphere suffixes; values plausible vs truth.

---

### depth — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| screen title "Depth" | Depth | yes | good | ok | OK | Centered cyan, strong contrast. |
| HERO = depth-below-keel (should be LABELED) | 2.7k m | yes | good | ok | MINOR | Correct scaling of 2670 m but no `below keel` sublabel — only screen title disambiguates. |
| TEMP sub-tile (value + C) | TEMP 18.5 | yes | good | ok | MINOR | Matches 18.5 C but `C` unit missing. |
| SOG sub-tile (value + kn) | SOG 4.4 | yes | good | ok | MINOR | Matches ~4.3 kn but `kn` unit missing. |
| TWA sub-tile (deg P/S) [intended true-wind compass, degraded to numeric] | TWA 144P | yes | good | ok | OK | Numeric port readout, plausible vs ~149° port. |

Combined:
- Overlaps: none — title above hero card, hero its own card, three sub-readouts (TEMP/SOG/TWA) in separate lower card, no collisions.
- Inconsistencies: HERO shows `m` but TEMP/SOG/TWA omit units (only 1 of 4 carries one); HERO value+unit decoupled (large gap, `m` vertically centered against digits, not a tight suffix); TWA `144P` inline P/S no degree, different style than numeric-only TEMP/SOG; hero tile has NO label at all, breaking the labeled-tile convention the sub-tiles establish.
- spaceUtilization: poorly balanced — HERO ~55% top with content floating upper-center, large empty band across lower ~40% of card; three sub-readouts crammed 2-then-1 (TWA alone on a second row, empty right half).
- clarity: HERO `2.7k` large/high-contrast/legible; sub-tile values legible; weak points semantic — hero unlabeled so below-keel vs below-transducer indistinguishable; `2.7k` + detached `m` easy to misread as `2.7 km`.
- problems:
  1. HERO not labeled as below-keel (expected) — no label at all; ambiguous vs below-transducer.
  2. Missing units on three of four values: TEMP no `C`, SOG no `kn`, TWA no degree.
  3. Large wasted empty space in lower half of HERO tile; value not vertically centered.
  4. TWA shown as bare `144P` instead of intended true-wind compass; orphaned on its own row.
  5. HERO `2.7k` + detached/offset `m` reads ambiguously (scaled-`k` + `m` unit hazard).
- whatWorks: clear hero + sub-readout hierarchy (sound MFD pattern); cyan hero vs white secondaries separates primary from context; no overlaps, consistent cards; values plausible vs truth.

Note: the four fixes targeting depth (HERO below-keel label) are NOT reflected in this
render — the hero still carries no label; see QA_COMPARISON.md.

---

### steering — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| HDG/CTS compass tile | HDG 039 (primary) + CTS 321 (amber secondary) | yes | good | ok | OK | Matches ~39.5°/321°; big digit lightly overlaps W/E ring labels but both readable. |
| XTE tile (m, k-scaled, P/S) [OVERFLOW] | 68.07kS | yes | good | clipped | PROBLEM | Value overflows right edge, leading digit + P/S clipped; reads `68.07kS` not full ~268k port; `m` pushed off-tile. |
| VMG tile (kn) | -3.7 kn | yes | good | ok | OK | Matches ~−3.8 kn, large cyan, no clipping. |
| RUDDER tile (deg + P/S helm) | 3P deg (+ -10/-1/+1/+10 helm buttons) | yes | good | ok | OK | Port matches −2.3° (rounds high), legible. |
| course-adjust row | -10 -1 +1 +10 | yes | good | clipped | MINOR | Present/legible but buttons flush against bottom frame, slightly clipped. |

Combined:
- Overlaps: XTE `68.07kS` overflows right edge clipped mid-`S`; `m` unit pushed entirely off-tile and invisible; run2 variant of same layout confirms systemic (`263.21kS m` colliding with right border); `k` scale-suffix + `S` side-suffix fused as `kS` (ambiguous token).
- Inconsistencies: VMG `kn` and RUDDER `deg` fit inside their tiles but XTE `m` clipped/absent; XTE fuses `k` inline (`68.07k`) while spec wants scaled value + separate `m`; RUDDER `3P` (letter-suffix) vs VMG `-3.7` (signed number) — two negative/port conventions; CTS secondary in amber is a one-off accent.
- spaceUtilization: four equal quadrants fill the canvas, course-adjust row tucked bottom; but VMG/RUDDER have large empty lower-right regions while XTE is over-full (overflows). XTE font mis-budgeted vs content width — wasted vertical whitespace in three tiles, horizontal overflow in the fourth.
- clarity: HDG/CTS, VMG, RUDDER legible and plausible vs truth; XTE value NOT clear — clipped at border, `m` missing, `kS` ambiguous; course-adjust buttons small but readable.
- problems:
  1. XTE value overflows tile, clipped at right edge (`68.07kS` cut mid-glyph); `m` unit pushed off-tile. Same overflow confirmed in run2 (`263.21kS m`). Value font not sized to fit tile width for large/k-scaled magnitudes.
  2. XTE scale-suffix + P/S suffix fused as `kS` (no unit between) — ambiguous; with clipping user can't read magnitude, side, or unit.
  3. Underlying degenerate sim route (XTE ~−268 km) is nonsensical even unclipped; no upper-bound/format guard — consider clamp/elide (`>99k`/`--`) and reserve room for the unit.
- whatWorks: clean balanced four-quadrant grid, consistent uppercase labels, legible cyan typography; HDG/CTS compass strong focal element (ticks + dual wind arrows + amber CTS); VMG/RUDDER show signed/port-suffixed values with trailing units; course-adjust row logically chunked; values plausible.

Note: combined cites a "run2" variant where XTE still collides with the right
border (`263.21kS m`) — the fit-shrink did not fully resolve steering XTE.

---

### route — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| XTE field (k-scaled, P/S) [OVERFLOW] | 68.06kS (run2: 263.21kS m) | yes | poor | clipped | PROBLEM | Leading `-2` clipped at left edge, unit mangled/overrun at right; degenerate −268 km unreadable and wrong side. |
| BTW bearing-to-waypoint (deg) | 339 | yes | good | ok | OK | Large cyan, fits cleanly; plausible bearing; no degree symbol though. |
| DTW distance-to-waypoint (nm) [OVERFLOW] | 453.95 nm (run2: 452.73 nm fits) | yes | good | clipped | PROBLEM | Oversized value overflows: leading digit clipped left, `nm` squeezed/clipped right. (run2 variant fits within tile.) |
| CTS course-to-steer (deg) | (none) | no | unreadable | misaligned | PROBLEM | No CTS tile; only DTW/BTW/XTE/VMG present — course-to-steer absent. |
| VMG (kn) | -3.8 kn (run2: -3.5 kn) | yes | good | ok | OK | Matches ~−3.8 kn, legible, fully contained. |
| route progress bar (if present) | (none) | no | good | ok | OK | No progress bar; conditional element, acceptable. |

Combined:
- Overlaps: XTE `k` scale-suffix and `S` side letter jammed (`263.21kS`) with no separator, then `m` trails after `S` — one run-on token; trailing `m` baseline-misaligned, tight against `S`, reads as part of the value.
- Inconsistencies: units inconsistent — DTW `nm`, XTE `m`, VMG `kn`, but BTW (`338`) has NO unit/degree; XTE encodes sign as side-letter `S` while VMG uses `-3.5`; XTE shows `k` inline + separate `m` (three-part token) while DTW shows no scale suffix at comparable digit count.
- spaceUtilization: 2x2 grid fills frame evenly, no dead zones; but under-populated for route use case — only 4 of 6 expected elements (CTS + progress bar absent); generous tile padding, values well-sized.
- clarity: DTW/BTW/VMG legible; XTE `263.21kS m` ambiguous (`S` could be unit/side/digit; `k` vs `m` muddled); BTW missing degree slightly hurts clarity.
- problems:
  1. XTE primary defect: truth −267769 m renders `263.21kS m` — magnitude wrong (263.21k vs ~268k, likely stale frame), side `S`(starboard) contradicts negative/port, and `k`+`S`+`m` concatenated into an unreadable token.
  2. Two expected elements missing entirely: CTS (~321°) and route progress bar; navigation-critical screen renders only 4 of 6 fields.
  3. BTW lacks degree symbol (`338`) — angular bearing indistinguishable from a count/distance.
  4. Inconsistent sign/unit conventions (side-letter XTE vs minus VMG; unit on DTW/XTE/VMG but absent on BTW).
  5. XTE side indicator derives side incorrectly for this degenerate route (shows starboard for a port/negative XTE) — verify P/S logic against signed value.
- whatWorks: clean equal-tile grid, full square, consistent margins; small-caps labels + large cyan values give strong panel feel; run2 fixed the horizontal value clipping on DTW/XTE (values now fit their tiles in the run2 variant); values plausible (DTW 452.73 nm, BTW 338, VMG −3.5/−3.8 kn).

---

### autopilot — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| compass dial + marker ring (HDG/COG/CTS/AP-target) | HDG 37.0°; COG 041° \| SOG 4.1 kn; N/E + ticks 30/60/120/330; markers clustered top | yes | good | crowded | MINOR | Dial plausible; HDG/COG/CTS/AP-target glyphs overlap at top (near-coincident bearings). |
| center HDG + COG/SOG sub-line | HDG 38.0°; COG 042° \| SOG 3.7 kn | yes | good | ok | OK | Large centered, legible, plausible vs truth. |
| XTE strip | PORT/STBD scale, marker pinned at PORT extreme | yes | good | ok | OK | Pegged hard PORT, consistent with −268 km XTE; clear, no overlap. |
| DEPTH tile [k-scaled vs over-precise] | DEPTH m 2667.7 (run2: 2.7k) | yes | good | ok | MINOR | Over-precise, NOT k-scaled on top render (should be `2.7k`); fills tile width vs neighbors. |
| SPEED tile | SPEED 3.8 kn | yes | good | ok | OK | Large white value + label/unit, plausible vs SOG/STW. |
| AWS tile | AWS kn 5.3 | yes | good | ok | OK | Amber on dark, labeled, legible, plausible. |
| AWA tile | 134P | yes | good | ok | OK | Port readout, plausible vs ~−127°; no degree/unit. |
| ON/STBY + HOME + MODE header | STBY \| AUTO (green) \| HOME | yes | good | ok | OK | Clean, no overlap; AUTO highlighted green for active mode. |

Combined:
- Overlaps: no hard box overlaps; soft overlap at dial top — AP-target/marker chevrons (orange + green) sit on the `30` tick label, crowding rim near 30–45°; DEPTH `2667.7` nearly spans tile width crowding right edge, `m` separated far top-right.
- Inconsistencies: DEPTH scaling differs between renders — top-level `2667.7` (raw/over-precise) vs run2 `2.7k` (scaling enabled, expected); decimal precision not normalized (DEPTH 0.1 m on ~2670 vs SPEED/AWS 0.1 at single-digit); angle notation mixed (dial/COG use degree, AWA `134P` no degree); P/S encoding mixed (AWA `P`, XTE `PORT/STBD`, others signs); unit placement mixed (tile units top-right grey vs dial inline `38.0°`).
- spaceUtilization: upper ~60% dial reads well; COG/SOG sub-line + XTE strip compact; unused vertical gap between SOG line and XTE strip; XTE strip functionally underused — normalized −1..1 with marker near center while truth is −268 km (not reflecting live value or silently clamped with no off-scale indication); bottom 4 tiles even except DEPTH's long number.
- clarity: header + HDG `38.0°` + COG/SOG sub-line strongest; DEPTH `2667.7` degraded by false precision (`2.7k` clearer); AWA `134P` ambiguous (no degree, `P` not self-evident); top marker chevrons small + overlap `30` label (can't tell HDG/COG/CTS/AP-target); CTS ~321° not obviously represented; XTE marker doesn't communicate the −268 km error.
- problems:
  1. DEPTH tile shows raw over-precise `2667.7 m` on the top-level render instead of k-scaled `2.7k` (run2 does it correctly) — false precision, inconsistent with the system.
  2. AWA `134P` lacks a degree symbol and uses an unexplained `P` suffix, inconsistent with dial/COG degree style.
  3. XTE strip does not visibly reflect the degenerate live XTE (−268 km); marker near center with no off-scale/clamp indication — misleading.
  4. Marker-ring chevrons (HDG/COG/CTS/AP-target) cluster on the `30` tick label, overlapping it and indistinguishable; CTS ~321° not clearly shown.
  5. Per-tile decimal precision not normalized (DEPTH 1 decimal at 2667 vs SPEED/AWS at single digits).
- whatWorks: strong hierarchy (mode header, large dial + centered HDG + COG/SOG sub-line, XTE strip, four supporting tiles); dial attractive/legible; values plausible vs truth; even tile layout; AUTO clearly highlighted; sensible color accents.

Note: the autopilot DEPTH k-scaling fix is present only in the "run2" variant the
reviewer cites; the top-level render still shows `2667.7`. See QA_COMPARISON.md.

---

### trip — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| trip distance (nm) | 203.0 nm | yes | good | ok | OK | Hero card, large white, no overlap/clipping, plausible. |
| elapsed time | 35:09:02 | yes | good | ok | OK | "TIME UNDERWAY" HH:MM:SS, legible bottom-left. |
| average speed | AVG SPEED 5.8 kn | yes | good | ok | OK | Clear label + value, center tile, plausible. |
| max speed | MAX SPEED 8.1 kn | yes | good | ok | OK | Green, legible bottom-right, plausibly above SOG/NOW. |
| NOW/current speed | NOW 4.1 kn | yes | good | ok | OK | Cyan, plausible vs SOG/STW ~4.3–4.4, no clipping. |

Combined:
- Overlaps: none.
- Inconsistencies: DISTANCE shows `nm` as a separate oversized right-aligned label far from value while stat/NOW tiles show units inline (`5.8 kn`); hero uses much larger font + value/unit split vs compact label-over-value tiles; accent color inconsistent (DISTANCE/AVG white, MAX/NOW teal — no single category meaning); unit string differs (`nm` vs `kn`, expected but styling amplifies mismatch).
- spaceUtilization: top-heavy — DISTANCE hero ~45% top, content centered with large empty margins; stat tiles + NOW bar well-packed; large empty gap between NOW bar and `console: trip-reset` footer (bottom ~15% unused).
- clarity: every box legible, clear small-caps labels, large readable values, no clipping; only interpretive issue is the inconsistent accent-color usage.
- problems:
  1. DISTANCE hero box wastes ~half its area as empty space; value could be larger/repositioned or box shortened.
  2. Inconsistent unit placement: hero detached oversized `nm` vs inline tile units.
  3. Inconsistent/ambiguous accent-color semantics: MAX/NOW teal vs DISTANCE/AVG white, no rule conveyed.
  4. Empty band between NOW bar and footer leaves the lower portion underused.
- whatWorks: clean hierarchy (distance hero, three-up stats, full-width NOW bar reading as live); all five elements present and populated; NOW 4.0 kn plausible vs SOG ~4.3; consistent dark theme, even spacing, no collisions; helpful footer hint; sensible HH:MM:SS + unit labels.

---

### status — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| WiFi state + SSID | WIFI: STA / SSID: yey-net | yes | good | ok | OK | Aligned label/value rows, legible white-on-dark, no clipping. |
| IP address | 10.42.0.67 | yes | good | ok | OK | Plausible private IPv4, cleanly aligned. |
| SignalK connection state | live | yes | good | ok | OK | SIGNALK row, good contrast (no color semaphore though). |
| device id/name | yey-d-28372f8a0290 | yes | good | ok | OK | On BLE row, legible, aligned, no clipping. |
| firmware build/version | BUILD: Jun 20 2026 | yes | good | ok | OK | BUILD row at bottom of SYSTEM list. |
| uptime | 00:07:01 | yes | good | ok | OK | UPTIME row, aligned, good contrast. |
| free heap | HEAP 22 kB | yes | good | ok | OK | Legible, aligned, plausible free heap. |
| SoC/Fuel/Water bars | SoC 86% (green), FUEL 99% (amber), WATER 100% (blue) + proportional bars | yes | good | ok | OK | Three color-coded percentage bars, proportional fills, no overlap. |

Combined:
- Overlaps: none — every row a clean label/value pair on its own baseline; the three bars sit right of values with clear separation.
- Inconsistencies: bars exist for only 3 of 14 rows (SoC/Fuel/Water) while BATTERY `13.78 V` and RSSI `-63 dBm` are also bounded/scalar but get none (bar treatment looks arbitrary); units mixed (`V`/`kB`/`dBm`/`%`/plain strings); `SoC` mixed-case vs all-caps neighbors; SoC green bar visibly shorter than 99/100% bars (reads truncated/mis-scaled).
- spaceUtilization: vertical used well (14 evenly-spaced rows fill the card); horizontal poorer — right ~40% empty for the 11 non-bar rows, large vertical dead band on the right.
- clarity: high — cyan SYSTEM title prominent, bordered card, strong label/value contrast, every row readable; bars give SoC/Fuel/Water at-a-glance status; SIGNALK `live` clear but not color-coded.
- problems:
  1. Large dead horizontal band on the right for the 11 non-bar rows — wasted ~40% width.
  2. SIGNALK live/stalled is plain white with no color semaphore — a stalled state would not stand out.
  3. Bar coverage inconsistent: only 3 bounded fields get bars, and SoC bar appears mis-scaled vs Fuel/Water (86% bar shorter than expected).
  4. Minor: device id/name only implicitly present as the BLE field; no explicit NAME/ID row.
- whatWorks: clean two-column ledger, consistent baselines, strong contrast, single accent title; all expected fields present and plausible (WiFi STA + yey-net, IP, SIGNALK live, BLE id, BUILD, UPTIME, HEAP, plus BATTERY/RSSI/PSRAM); tank/charge bars add quick scan; nothing overlaps or clips.
