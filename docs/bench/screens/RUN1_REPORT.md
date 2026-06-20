# Screen Visual QA — Run 1 (pre-fix)

Per-element + combined visual QA review of all 11 device screens against expected element lists.

## Summary

| Screen | Verdict | # problems | Top problem |
|---|---|---|---|
| dashboard | MINOR | 3 | WIND tile missing its `kn` unit label (only `5.7` rendered) |
| wind | PROBLEM | 4 | Cardinal labels rotate with bezel instead of staying upright |
| wind_classic | PROBLEM | 6 | Wind markers collide with rim labels (gold square over `W`, glyph over `150`) |
| wind_steer | PROBLEM | 5 | XTE strip overlaps the wind-tile row (PORT/STBD labels + indicator collide) |
| nav | MINOR | 6 | Large `040` HDG number hides the W and E cardinal markers |
| depth | PROBLEM | 6 | HERO not labeled as `below-keel` (ambiguous depth reference) |
| steering | PROBLEM | 5 | XTE tile clips value on both edges; auto-fit fails on wide k-scaled string |
| route | PROBLEM | 5 | XTE value overflow — sign/leading digits clipped, magnitude/polarity lost |
| autopilot | MINOR | 4 | DEPTH tile bypasses k-scaling, shows over-precise edge-clipped `2667.7 m` |
| trip | MINOR | 5 | DISTANCE box oversized — wastes top ~45% on one number |
| status | MINOR | 4 | Large wasted empty space in right half below the WATER row |

---

### dashboard — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| WIND round dial tile (AWS + kn) | 5.7 | yes | good | ok | MINOR | Legible orange value but no `kn` unit label, unlike SOG. |
| SOG tile (value + kn) | 4.3 kn | yes | good | ok | OK | Large cyan value with unit, matches ground truth. |
| DEPTH tile (value + m, k/M scaled) | 2.7k m | yes | good | ok | OK | Correctly scales 2670m; good contrast, no clipping. |
| BATT tile (percent + fill bar) | 86% + green bar | yes | good | ok | OK | Proportional ~86% green bar, clean placement. |

Combined:
- Overlaps: none.
- Inconsistencies: four unit conventions on one screen (SOG `kn`, DEPTH `m`, BATT inline `%`, WIND bare value, no unit); differing value/unit visual treatment; WIND value orange while others cyan (unexplained); WIND value centered in ring vs others left-aligned (no shared baseline/margin).
- spaceUtilization: Clean even 2x2 grid filling the 480x480 panel; WIND tile well used by dial; SOG/DEPTH leave bottom ~40% blank — headroom but not broken.
- clarity: Clear uppercase titles and large high-contrast values; only gap is the WIND tile's tiny ambiguous markers and missing unit.
- whatWorks: Balanced, scannable 2x2 grid; consistent titles; large high-contrast values; DEPTH k/M scaling correct; SOG matches truth; BATT bar gives at-a-glance read.
- problems:
  1. WIND tile missing its unit label entirely (only `5.7`, no `kn`).
  2. WIND value plausibility: dial shows 5.7 vs ground-truth AWS 6.0 kn (likely sampling lag, worth flagging).
  3. WIND dial markers very small/noisy (amber triangle + green tick clipped at lower-left of ring); meaning unclear, sit awkwardly near tile edge.

---

### wind — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| rotating compass bezel + ticks | ring rotated to 040/NE under pointer | yes | good | ok | OK | Centered, legible, correctly oriented to HDG ~040. |
| cardinal labels (MUST stay upright) | N/NE/E/SE/S/SW/W/NW | yes | good | ok | OK | (per-element) labels upright and non-overlapping. |
| center HDG number | 040° | yes | good | ok | OK | Large light-blue, matches ~39.5°, no clipping. |
| apparent-wind marker | orange marker on WSW rim | yes | good | ok | OK | Correct for AWA -134 port vs HDG 040. |
| true-wind marker | cyan `T` index on lower-left rim | yes | good | ok | OK | Consistent with TWA ~149 port; distinct from `A`. |
| AWS bottom tile (kn) | 6.3 | yes | good | ok | OK | Orange, labeled, plausible vs 6.0kn. |
| AWA bottom tile (deg P/S) | 134P | yes | good | ok | OK | Correct port suffix, matches ~-127 port. |
| TWS bottom tile (kn) | 9.8 kn | yes | good | ok | OK | Clear labels, consistent with 9.3 kn. |
| TWA bottom tile (deg P/S) | 153P | yes | good | ok | OK | Plausible vs -149 port, clean. |

Combined:
- Overlaps: unlabeled center `0.8` + blue wind-vector arrow crowd the boat-hull graphic; top-center congested (HDG caption + red chevron + green/red arc cluster); rim glyphs (orange `A`, cyan `T`) crowd the `W`/`SW` cardinal labels.
- Inconsistencies: cardinal labels are NOT upright — rotated to follow bezel tangent (spec violation); unit handling differs (AWS/TWS show `kn`, AWA/TWA omit `deg`, use P/S); center `0.8` has no caption/unit; unexplained gold/yellow square marker on left rim with no legend.
- spaceUtilization: Good — dial fills upper ~70%, four bottom tiles evenly spaced full width; only congestion is dial center and top arc.
- clarity: Bottom tiles highly legible; HDG `040` correct. Weak: center `0.8` small/unlabeled/ambiguous; multiple rim markers lack a legend.
- whatWorks: Four bottom tiles excellent (consistent grid, plausible values matching live data); green ring + tick bezel strong; HDG prominent and correct; boat-centric metaphor intuitive.
- problems:
  1. Cardinal/intercardinal labels rotate with bezel instead of staying upright — direct spec violation; tilted labels hard to read.
  2. Center `0.8` is unlabeled, unitless, and implausible (matches no ground-truth quantity) — likely wrong field/scaling bug; also overlaps boat icon.
  3. No legend distinguishing apparent- vs true-wind rim markers (3 colored markers + stray gold square).
  4. Bottom AWA/TWA tiles omit the `deg` unit; reader must infer from P/S suffix.

---

### wind_classic — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| rotating bezel + ticks | cardinals + 30/60/90/120/150 graduations | yes | good | ok | OK | Legible, rotated consistent with HDG 036. |
| cardinal labels (MUST stay upright) | N/E/S/W | yes | good | ok | OK | All four upright, not rotated — requirement met. |
| in-dial hero readouts | HDG 056°, AWS 12.4, TWS 14.5, SOG 6.2, SOW 5.7 | yes | good | ok | OK | Large high-contrast; minor gap = tiny `1.4`, absent COG/STW. |
| wind markers on rim | color-coded glyphs + blue AW arrow | yes | good | ok | OK | Distinct/unclipped, consistent with 124P/145P angles. |
| unit labels on hero readouts | `kn` on SOG/SOW, `°` on HDG; AWS/TWS none | yes | good | ok | OK | Present and legible; AWS/TWS omit speed unit by design. |

Combined:
- Overlaps: gold square marker overlaps upright `W` label; white glyph overlaps `150` tick label; center `0.8` floats over boat hull and abuts blue needle origin; `S` label nearly touches SOW corner box, a marker glyph partly clipped behind SOG box; SOG box top-right touches bezel ring arc.
- Inconsistencies: three unit conventions (HDG baked-in degree, AWS/TWS no unit, SOG/SOW print `kn`); angle subvalues `124P`/`145P` use P/S, no degree symbol; center `0.8` no label/unit; two readout styles (titled+unit corner cards vs untitled in-dial heroes); bottom-right labeled `SOW` (nonstandard; conventional term is STW).
- spaceUtilization: Upper/middle bands well used; lower half largely empty (only boat icon + bezel); corner SOG/SOW boxes sit in dead bottom corners but break circular composition and crowd the rim instead of using open lower-center space.
- clarity: Hero readouts + SOG/SOW boxes large/legible; color coding distinguishes channels. Mystery center `0.8` unclear; small markers collide with labels reducing clarity.
- whatWorks: Cardinal labels render UPRIGHT on the rotating bezel (key requirement met); close-hauled red/green arc, tick ring, big heroes with P/S subvalues, blue AW needle, color-coded SOG/SOW cards all read clearly; values plausible vs truth.
- problems:
  1. Wind markers collide with rim labels (gold square over `W`, white glyph over `150`) — need radius/offset avoiding cardinal/tick text.
  2. Center `0.8` readout unlabeled/unitless, floating over boat hull and needle origin.
  3. Inconsistent unit policy (AWS/TWS omit `kn` while SOG/SOW show it, HDG shows degrees).
  4. Corner SOG/SOW boxes intrude on bezel/marker rim and clip a marker glyph; break circular dial aesthetic.
  5. Expected in-dial heroes COG and STW missing/relabeled — COG absent, STW shown as nonstandard `SOW` in a corner box.
  6. Lower-center dial space wasted while corners are crowded.

---

### wind_steer — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| compass dial + no-go/target sectors | HDG 36.2° dial, green target + red no-go | yes | good | ok | OK | Clean horseshoe dial, no clipping/overlap. |
| marker ring (HDG/COG/CTS/TWD bugs) | red HDG/COG bug + N/E cardinal bugs | yes | good | ok | OK | Legible on white arc; top cluster slightly busy. |
| center HDG + TWA sub-line | HDG 36.2°, "TWA 139°P \| TWD 257°" | yes | good | ok | OK | Centered, large, legible, no clipping. |
| XTE strip (should show numeric value) | PORT/STBD scale -1.0..1.0, marker ~+0.15 | yes | good | ok | MINOR | Legible bar but no numeric cross-track value as required. |
| AWS/AWA/TWS/TWA wind tiles | AWS 8.3kn, AWA 113P, TWS 11.7kn, TWA 139P | yes | good | ok | OK | Clean 4-column row, labeled, plausible. |
| ON/STBY button | STBY | yes | good | ok | OK | Clear white text, good contrast. |
| HOME button | HOME | yes | good | ok | OK | Top-right rounded-rect, legible. |
| course-adjust row | -10 -1 +1 +10 | yes | good | ok | OK | Bottom row pills, legible, not clipped. |
| TACK button | TACK | yes | good | ok | OK | Well-spaced in bottom control row. |
| GYBE button | GYBE | yes | good | ok | OK | Present, legible, no overlap. |

Combined:
- Overlaps: (severe) XTE strip vertically crushed against AWS/AWA/TWS/TWA row — `PORT` label and red indicator render on the AWS tile's top border, center tick/`0` fall into the AWA/TWS gap; (severe) marker-ring bug cluster near top (~30-55°) — red chevron, green pill, blue trapezoid, green triangle mutually overlap, indistinguishable; bug cluster crowds the `30` tick label; `STBD` label flush on TWA tile top-right corner.
- Inconsistencies: XTE shows NO numeric value (live XTE -267769 m = -268 km absent) — the only metric reduced to bar-only; wind tiles label only half (AWS/TWS `kn`, AWA/TWA none); angle formatting differs between center sub-line (degree+side) and tiles (bare).
- spaceUtilization: Top-heavy — dial dominates upper ~60% (well used) but the band between dial and tiles is starved, forcing overlap; tiles are tall with large empty interiors. Reclaiming ~12-16px of tile height would give the strip its own band. Course-adjust row compact and appropriate.
- clarity: Center HDG large/crisp; TWA/TWD sub-line clean; tiles individually legible. Two problem zones: marker-ring bug cluster (illegible overlap) and XTE strip (cramped + missing numeric value). Bottom button rows clear.
- whatWorks: Strong hierarchy (big center HDG, supporting sub-line, color-coded band, dark theme); values plausible and track truth; all expected controls present and correctly placed; button rows evenly spaced and finger-sized.
- problems:
  1. XTE strip overlaps the wind-tile row (PORT/STBD labels, red indicator, `0` tick collide with tile top edges) — give the strip a dedicated band.
  2. XTE strip displays no numeric cross-track value; add a numeric readout with k/M scaling; clamp/flag off-scale values rather than pinning near center.
  3. Marker-ring bugs (HDG/COG/CTS/TWD) collapse into an unreadable cluster when headings converge — needs de-clutter/spread or stacking-priority logic.
  4. Unit-label inconsistency across wind tiles (`kn` on AWS/TWS, none on AWA/TWA, degree glyph dropped).
  5. Wasted vertical space inside tall tiles while strip above is starved — rebalance.

---

### nav — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| position lat/lon (DDM) | 37°53.310'N / 017°44.960'E | yes | good | ok | OK | Well-formatted DDM in cyan, no clipping. |
| COG value | 045 | yes | good | ok | OK | Large legible, plausible vs ~43°; echoed in dial. |
| SOG value | 3.5 kn | yes | good | ok | OK | Large cyan + unit, near ~4.3kn truth. |
| HDG value | 040 | yes | good | ok | OK | Large `040` in compass rose, matches ~39.5°. |
| mini compass/heading indicator | 040 center, N/S/W marks, "COG 045" | yes | good | ok | OK | Circular dial with pointers and sub-label, legible. |

Combined:
- Overlaps: oversized `040` overlaps the W and E cardinal letters (W partly covered by leading zero, E almost fully obscured) — only N and S legible; HDG-tile marker glyphs (red arrow, green/cyan pair) crowd the `HDG` label, red glyph nearly touches its baseline.
- Inconsistencies: three unit conventions (SOG `kn`, COG/HDG bare numbers, POS degree/minute symbols); COG duplicated (big tile `045` + small orange `COG 045` caption in compass); headings lack degree symbol while POS uses symbols; uneven color (in-compass `COG 045` is the only orange element, no legend); value font sizes differ between compass-embedded and standalone numbers.
- spaceUtilization: Reasonable 2x2 grid with even gutters but uneven within tiles — SOG/COG have large empty right/bottom regions (left-aligned values); POS well-filled; HDG tile overcrowded (rose + number + glyphs + caption); inconsistent alignment.
- clarity: SOG/COG/POS clear. HDG tile weak — number legible but rose loses two cardinal letters; small orange `COG 045` low contrast; marker glyphs unlabeled.
- whatWorks: Clean 2x2 card grid, dark marine theme, consistent grey uppercase labels; POS nicely formatted DDM; SOG instantly readable; values plausible vs truth; contains exactly expected elements.
- problems:
  1. Large `040` overlaps/hides the W and E cardinal markers, defeating the heading rose.
  2. Redundant/duplicated COG (own tile `045` + orange caption `COG 045`) — wastes space, two styles for one datum.
  3. Missing degree symbols on HDG/COG — inconsistent with POS.
  4. Wasted whitespace in SOG/COG tiles while HDG tile overcrowded — unbalanced density.
  5. SOG reads 3.5 kn vs ground-truth ~4.3 kn — verify source/scaling (may be stale frame).
  6. Compass marker glyphs unexplained and crowd the `HDG` label.

---

### depth — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| screen title "Depth" | Depth | yes | good | ok | OK | Centered cyan, good contrast. |
| HERO = depth-below-keel (should be LABELED) | 2.7k m | yes | good | ok | MINOR | Correct scaling of 2670m but no `below-keel` label. |
| TEMP sub-tile (value + C) | TEMP 18.5 | yes | good | ok | MINOR | Matches 18.5C truth but `C` unit missing. |
| SOG sub-tile (value + kn) | 4.4 | yes | good | ok | MINOR | Matches ~4.3kn but `kn` unit missing. |
| TWA sub-tile (deg P/S) | TWA 144P | yes | good | ok | OK | Numeric port readout, plausibly tracks -149 port. |

Combined:
- Overlaps: no hard pixel overlaps, but HERO value `2.7k` and unit `m` separated by a large gap (~50px) — reads as a disconnect rather than clean value+unit pairing.
- Inconsistencies: unit display inconsistent (HERO shows `m`; TEMP/SOG/TWA show none — spec calls for `C`/`kn`); label/value semantics inconsistent (HERO not labeled `below-keel` and has no caption at all, while every sub-tile has one — the most important value is the least labeled); color treatment inconsistent (HERO value cyan but its `m` unit muted grey, sub-tile values white); TWA uses compound `144P` with no separator/degree mark.
- spaceUtilization: Poorly balanced — HERO occupies top ~55% with a single short token vertically centered, leaving large empty bands above/below; three sub-tiles crammed into bottom ~40% in a 2-col-then-wrap arrangement; TWA sits alone on a second row with an empty right-hand quadrant (orphaned cell).
- clarity: Each box individually legible, but clarity of MEANING degraded — `2.7k m` with no qualifier doesn't communicate depth-below-keel; scaled `2.7k` looks alarming/odd for depth; TWA `144P` cryptic without degree symbol/compass context.
- whatWorks: Two-tier hero + sub-tile hierarchy sound and readable; centered cyan title clean; rounded cards separate regions; hero appropriately dominant; all live values plausible vs truth; caption-over-value pattern on sub-tiles consistent.
- problems:
  1. HERO not labeled as `below-keel` as required — bare `2.7k` is ambiguous (below-keel vs below-transducer vs below-surface differ).
  2. Sub-tile units missing: TEMP no `C`, SOG no `kn`, TWA no degree symbol (spec explicitly expects these).
  3. Excessive empty vertical space inside hero while sub-grid is cramped.
  4. Orphaned empty cell: TWA alone on bottom row leaves bottom-right quadrant blank (unbalanced 3-into-4 grid).
  5. Large semantically-meaningless gap between hero value `2.7k` and its unit `m`.
  6. Unit-color mismatch: hero `m` is dim grey and detached from its cyan value.

---

### steering — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| HDG/CTS compass tile | HDG 037, CTS 321 | yes | good | ok | OK | Large `037` + yellow `CTS 321`, legible top-left. |
| XTE tile (m, k-scaled, P/S) [OVERFLOW] | 68.07kS (overflowing; expected ~268k P) | yes | poor | clipped | PROBLEM | Leading digit cut at left, unit/suffix clipped at right. |
| VMG tile (kn) | -3.7 kn | yes | good | ok | OK | Matches -3.8kn, clear, no clipping. |
| RUDDER tile (deg + P/S helm) | 3P deg (+1/+10 helm buttons) | yes | good | ok | OK | Port matches -2.3° (magnitude rounds 2.3→3). |
| course-adjust row | -10 -1 +1 +10 | yes | good | ok | OK | Four chips, well-spaced, no clipping. |

Combined:
- Overlaps: no element-to-element overlap, but XTE value overflows its own tile on BOTH horizontal edges (leading digit clipped at left border, trailing `S` clipped at right); course-adjust row sits flush at the very bottom edge of VMG/RUDDER tiles with almost no vertical breathing room.
- Inconsistencies: unit-label presence inconsistent (VMG `kn`, RUDDER `deg`, XTE no `m` label at all, HDG/CTS no degree unit); value formatting differs (`037` zero-padded vs `-3.7`/`3P` not); P/S suffix rendered as primary-size glyph vs small secondary unit labels; sign convention surfaces differently (VMG numeric `-`, RUDDER/XTE letter side-code); ground-truth sign mismatch — XTE is -267769 m (port) but suffix shows `S` (starboard), RUDDER -2.3° port renders `3P`.
- spaceUtilization: Uneven — HDG/CTS well-filled; XTE overfilled to overflow (auto-fit did not shrink the degenerate -268km string); VMG/RUDDER have large empty right/below regions; button row crammed into bottom strip. Top-right crowded/clipped, bottom-right under-utilized.
- clarity: HDG/CTS, VMG, RUDDER individually legible and labeled. XTE NOT clear — overflows both edges so magnitude unreadable, no visible `m` unit.
- whatWorks: Four-tile 2x2 grid with docked course-adjust row is sensible/readable; HDG/CTS compass (rose + side-vane markers + `037` + amber `CTS 321`) strongest element; color hierarchy consistent (cyan values, muted labels, amber secondary); VMG/RUDDER clean hierarchy.
- problems:
  1. XTE tile clips value on both horizontal edges; auto-fit/font-shrink fails on the wide k-scaled string `268.07kS` — needs smaller-font fallback, narrower formatting (drop decimals at k/M → `268k`), or ellipsis/scientific handling.
  2. XTE `m` unit label missing/clipped — scale not communicated; `S` suffix half-clipped at right.
  3. XTE side-letter (`S` starboard) contradicts port ground truth (-267769 m) — sign-to-side mapping looks inverted.
  4. Course-adjust buttons sit too close to bottom tile edges (insufficient padding) — cramped.
  5. Inconsistent unit/suffix typography: side-codes (P/S) render at full value size while units (kn/deg) render small.

---

### route — PROBLEM

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| XTE field (k-scaled, P/S) [OVERFLOW] | (none) | no | unreadable | clipped | PROBLEM | Screenshot path resolved to literal `undefined/route.png` — file absent; field could not be evaluated. |
| BTW bearing-to-waypoint (deg) | 339 | yes | good | ok | OK | Plausible bearing near ~321 CTS, no clipping. |
| DTW distance-to-waypoint (nm) [OVERFLOW] | 453.95 nm | yes | good | clipped | PROBLEM | Leading `4` clipped at left edge, `nm` truncated against border. |
| CTS course-to-steer (deg) | (none) | no | unreadable | ok | PROBLEM | No CTS tile present; screen shows only DTW/BTW/XTE/VMG. |
| VMG (kn) | -3.8 kn | yes | good | ok | OK | Matches -3.8kn truth, fully contained. |
| route progress bar (if present) | (none) | no | unreadable | clipped | PROBLEM | Screenshot `undefined/route.png` absent — could not inspect. |

Note: The screenshot file resolved to literal `undefined/route.png` and does not exist on disk; some elements were assessed from the combined view while XTE/progress-bar could not be directly inspected.

Combined:
- Overlaps: XTE tile — `k` scale suffix and `S` side indicator jammed (`kS`) with no separation, reads as a garbled token; DTW tile — `nm` unit sits against the right edge and is clipped to `nn`, overlapping the value bounding area.
- Inconsistencies: unit display inconsistent (VMG `kn`, DTW broken `nm`, BTW `339` no degree, XTE no `m`) — two distance fields treated differently; value alignment inconsistent (left column DTW/XTE left-anchored and overflow left edge; right column BTW/VMG comfortably inset); expected field set inconsistent — CTS absent, no progress bar, only 4 of 5 core fields fit the 2x2 grid.
- spaceUtilization: Mediocre — rigid 2x2 grid holds 4 fields while the screen needs 5 (CTS dropped) plus optional progress bar; oversized font directly causes left-edge clipping in DTW/XTE; generous gutter/margin could have been reduced to give text horizontal room. Font sized too aggressively for worst-case widths.
- clarity: BTW (`339`) and VMG (`-3.8 kn`) fully legible. DTW mostly readable but leading `4` shaved and unit reads `nn`. XTE worst — renders `68.06kS` with sign/leading digits clipped off left edge, magnitude and polarity unreadable, `kS` tail ambiguous (scale-k vs starboard-S).
- whatWorks: Clean consistent tile style (rounded dark cards, cyan values, grey top-left labels, even spacing); BTW and VMG demonstrate the intended design working; VMG correct sign/unit matching truth; BTW plausible; label-top/value-center layout readable with strong contrast.
- problems:
  1. XTE value overflow (the flagged risk) — live XTE -267769 m (-268 km, degenerate sim route); should read ~`-268.06k` but renders `68.06kS` with sign and leading `2` clipped off left edge; magnitude/polarity lost; `m` unit missing; `S` side flag collides with `k` suffix.
  2. DTW unit clipping — `nm` clipped to `nn` at right edge; value's leading digit also shaved at left.
  3. Value text not constrained to its tile — left-column values bleed past left card border; need auto-fit or right-alignment within the cell.
  4. CTS (course-to-steer) missing entirely — one of the expected route fields not rendered; no progress bar present.
  5. XTE polarity/side encoding broken — P/S indicator (`S`) mashed onto the scale suffix; port/starboard side not clearly conveyed.

---

### autopilot — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| compass dial + marker ring | HDG 38.0°; markers HDG/COG/CTS/AP-target; "COG 042° \| SOG 3.7 kn" | yes | good | overlap | MINOR | Renders clearly; four marker glyphs cluster/overlap at top (HDG~COG near-equal) but stay distinguishable. |
| center HDG + COG/SOG sub-line | HDG 38.0°; COG 042° \| SOG 3.7 kn | yes | good | ok | OK | Centered, legible, plausible vs truth. |
| XTE strip | PORT/STBD scale, red needle pegged ~-1.0 | yes | good | ok | OK | Pegged hard to port, consistent with -268km XTE. |
| DEPTH tile [k-scaled vs over-precise] | 2667.7 m | yes | good | clipped | PROBLEM | Raw over-precise (not k-scaled to ~2.7k); value fills/touches right edge. |
| SPEED tile | 3.8 kn | yes | good | ok | OK | Clean, plausible vs STW/SOG. |
| AWS tile | 5.3 kn | yes | good | ok | OK | Amber, legible, plausible vs 6.0kn. |
| AWA tile | 134P | yes | good | ok | OK | White-on-dark, plausible vs -127 port. |
| ON/STBY + HOME + MODE header | STBY \| AUTO (green) \| HOME | yes | good | ok | OK | Evenly spaced, no overlap. |

Combined:
- Overlaps: XTE red marker railed at far-PORT (-1.0) collides with the `PORT` label above and the `-1.0` tick label below; compass top — COG/CTS/AP-target glyphs stacked in a muddle near 38-43° (HDG~39.5, COG~43, AP target near-coincident), hard to distinguish; DEPTH value `2667.7` runs to the very right tile edge, trailing `7` nearly clipped (no padding).
- Inconsistencies: DEPTH shown full-precision and UN-scaled (`2667.7 m`) though scaling should render `2.7k` — the one tile ignoring k/M convention; DEPTH over-precise (0.1m on ~2.7km is meaningless, causes overflow); unit labels inconsistent (DEPTH `m`, SPEED/AWS `kn`, AWA no unit/degree); angle formatting inconsistent (sub-line uses degree symbol, AWA uses `134P` no degree, bare `P`); AWS 5.3 vs truth 6.0 and AWA ~134 vs 127 (minor, P/port correct).
- spaceUtilization: Reasonable — 180° dial dominates upper two-thirds; empty band between COG/SOG sub-line and XTE strip; dial's flat ends leave dead corners; bottom 4-tile row well packed; XTE strip thin/underused (only normalized bar, no numeric magnitude).
- clarity: Header clear; center HDG `38.0` large; COG/SOG sub-line readable; SPEED/AWS crisp. Weak: DEPTH cramped/edge-clipped and un-scaled; AWA `134P` ambiguous without degree; clustered top markers have no legend (can't tell COG vs CTS vs AP-target).
- whatWorks: Compass dial (cardinal letters, ticks 30/60/120/330, green active arc) attractive with clear focal HDG; STBY/AUTO(green)/HOME header clean and balanced; four-tile bottom strip sensible; XTE bar correctly saturates to PORT for degenerate -268km route; purposeful color coding.
- problems:
  1. DEPTH tile bypasses k-scaling — shows `2667.7 m` over-precise and edge-clipped instead of `2.7k m` (consistency bug + visual overflow).
  2. AWA tile lacks a unit/degree symbol while siblings show units, and uses a bare `P` suffix — breaks label convention.
  3. Top compass markers (COG/CTS/AP-target) overlap into an indistinguishable cluster with no legend when headings are close.
  4. XTE marker line overlaps PORT and -1.0 text labels; strip provides no numeric XTE value, so magnitude of the saturated error is invisible.

---

### trip — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| trip distance (nm) | 203.0 nm | yes | good | ok | OK | Hero tile, right-aligned `nm`, large high-contrast. |
| elapsed time | 35:07:51 | yes | good | ok | OK | "TIME UNDERWAY", large mono-style, lower-left. |
| average speed | 5.8 kn | yes | good | ok | OK | "AVG SPEED" tile, clear label, good contrast. |
| max speed | 8.1 kn | yes | good | ok | OK | Green text, plausible peak above ~4.3kn SOG. |
| NOW/current speed | 4.1 kn | yes | good | ok | OK | Full-width bar, plausible vs SOG~4.3/STW~4.4. |

Combined:
- Overlaps: none.
- Inconsistencies: unit treatment inconsistent (DISTANCE `nm` as large detached right glyph vs speeds inline `5.8 kn`); TIME UNDERWAY `35:07:51` carries no unit/format hint while speeds carry `kn`; color coding not rule-driven (DISTANCE/TIME/AVG white, MAX green, NOW cyan — no semantic reason MAX is green); font hierarchy uneven (DISTANCE ~64px, second row ~28px, NOW mid-size — the most operationally-relevant NOW is smaller than the cumulative DISTANCE).
- spaceUtilization: Top-heavy — DISTANCE box consumes upper ~45% for one number with vast empty space left of `203.0` and between value and `nm`; second row + NOW strip reasonably packed; footer `console: trip-reset` hint leaves dead space at very bottom.
- clarity: Every box individually legible; clear section labels; good contrast; no clipping; all 5 expected elements present; rounded cards separate regions cleanly.
- whatWorks: All five elements present, labeled, plausible vs truth; sound hierarchy concept (hero on top, three summary stats, live NOW strip, console-command footer); card grouping with captions readable; clean contrast/alignment, no overlaps.
- problems:
  1. DISTANCE box heavily over-sized for content — large gap between `203.0` and right-aligned `nm`, wastes top ~45% on one number.
  2. Inconsistent unit rendering: `nm` big detached glyph vs small inline `kn`.
  3. MAX SPEED green vs AVG SPEED white has no on-screen legend/rule — highlight reads as arbitrary.
  4. TIME UNDERWAY lacks any format hint; `35:07:51` relies on viewer to infer hh:mm:ss.
  5. Live NOW value (most glance-critical) rendered smaller/lower-prominence than cumulative DISTANCE — hierarchy arguably inverted for an at-a-glance trip screen.

---

### status — MINOR

| Element | Shown | Found | Legible | Placement | Verdict | Note |
|---|---|---|---|---|---|---|
| WiFi state + SSID | WIFI: STA, SSID: yey-net | yes | good | ok | OK | Adjacent label/value rows, aligned, no clipping. |
| IP address | 10.42.0.67 | yes | good | ok | OK | Plausible address, cleanly aligned. |
| SignalK connection state | live | yes | good | ok | OK | "SIGNALK" row, well-aligned, good contrast. |
| device id/name | yey-d-28372f8a0290 | yes | good | ok | OK | BLE row, legible, aligned, no clipping. |
| firmware build/version | Jun 20 2026 | yes | good | ok | OK | BUILD row at bottom of SYSTEM list. |
| uptime | 00:07:01 | yes | good | ok | OK | UPTIME row, aligned, good contrast. |
| free heap | 22 kB | yes | good | ok | OK | HEAP row, white-on-dark, clean. |
| SoC/Fuel/Water bars | SoC 86% (green), FUEL 99% (orange), WATER 100% (blue) | yes | good | ok | OK | Three labeled rows with proportional color bars. |

Combined:
- Overlaps: none detected — labels (~x35), values (~x135), and bars (~x285-460) occupy distinct columns; widest value (`yey-d-28372f8a0290`) clears the right edge cleanly.
- Inconsistencies: unit handling inconsistent (BATTERY `13.78 V`, RSSI `-63 dBm`, HEAP `22 kB`, PSRAM `7314 kB` carry units; SoC/FUEL/WATER bare `%`; others unitless by nature) — HEAP `kB` and PSRAM `kB` span very different magnitudes (22 vs 7314) with no thousands separator on PSRAM; only top three rows have bars while BATTERY (a bounded gauge) has none in the same band; `SIGNALK` rendered one word vs brand `SignalK`; bars start at a different x than any text (two alignment grids in right ~40%).
- spaceUtilization: Vertical space used well (15 rows evenly distributed in a rounded card); horizontal underused on lower ~11 rows — entire right half empty below WATER; value column short, leaving an empty middle-right gutter; layout reads left-weighted with a big empty bottom-right rectangle.
- clarity: Every row highly legible (high-contrast white on dark navy, cyan title, distinct bar colors); consistent readable font; no clipping; all expected status fields present and unambiguous.
- whatWorks: Clean scannable label/value list with consistent uppercase labels and aligned values; clear rounded card framing; sensible field ordering (power/resources top, network middle, identity/firmware bottom); three capacity bars are the strongest visual element; values sane for the ESP32-S3-N16R8.
- problems:
  1. Large wasted empty space in the right half below the WATER row — bottom ~11 rows leave ~40% of width blank, screen feels left-weighted/unbalanced.
  2. WiFi state and SSID split across two rows (plus RSSI a third) — could be consolidated to free vertical room or allow larger type.
  3. HEAP `22 kB` free internal heap is plausibly low/near a warning threshold (NimBLE/PSRAM-sensitive) yet has no visual emphasis (color/threshold) — a degraded heap/SignalK state would not stand out.
  4. PSRAM `7314 kB` lacks a thousands separator or M-scaling — inconsistent with the firmware's documented k/M value-scaling behavior used elsewhere.
