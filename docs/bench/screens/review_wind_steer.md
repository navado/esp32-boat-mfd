# Visual QA — wind_steer.png

Resolution 480x480. Ground-truth snapshot: HDG 39.5°, COG 43.3°, AWA -127.5°P,
AWS 6.0 kn, TWA -149.4°P, TWS 9.3 kn, CTS ~321°, XTE -268 km (sim-degenerate).

## Displayed values (5 questions each)

### Top button row: STBY (left), AUTO (center, green), HOME (right)
- (1) Plausible — autopilot mode pill. OK
- (2) No unit needed. OK
- (3) N/A. OK
- (4) Labels clear; AUTO highlighted green = active state, STBY/HOME framed buttons. OK
- (5) Legible, no overlap. OK

### Compass dial — HDG 36.2°
- (1) Ground truth 39.5°; 36.2° is within plausible wander. OK
- (2) Degrees (°) shown. OK
- (3) One decimal — fine. OK
- (4) Label "HDG" above number. OK
- (5) Large, crisp, centered. Cardinal letters N/E shown on rose. OK
- NOTE: the no-go (red) / target (green) sectors and the marker ring render;
  needle/markers point upper area. Plausible.

### Center sub-line — "TWA 139°P | TWD 257°"
- (1) TWA ground truth -149.4°P → magnitude 149; shown 139°P. Off by ~10° but
  within wander/lag tolerance, sign (Port) correct. OK
- (1) TWD 257°: TWA -149°P relative to HDG ~36° gives TWD ≈ 36-149 = -113 → 247°.
  257° plausible. OK
- (2) Degrees + P suffix. OK
- (3) Integer degrees — fine. OK
- (4) Labels clear. OK
- (5) Legible. OK

### XTE strip — "PORT ... STBD" band under the dial
- (1) XTE is absurdly large (-268 km from degenerate sim). The strip shows only
  PORT / STBD end labels with a centered tick — NO numeric XTE value is printed.
- (2) N/A (no value shown).
- (3) PROBLEM — the strip degrades to just side labels; the actual cross-track
  magnitude is never displayed, so a user cannot read XTE at all. With the
  absurd sim value this is arguably graceful, but a legitimate XTE would also
  be invisible.
- (4) PORT/STBD labels are tiny and crowd the dial's lower lobes. MINOR.
- (5) Faint, low contrast, partly tucked behind compass lobes. MINOR.

### Wind tile 1 — AWS 8.3 kn
- (1) Ground truth AWS 6.0 kn; shown 8.3. Off by ~2.3 kn — larger than typical
  wander. MINOR concern (could be apparent-vs-snapshot timing). MINOR.
- (2) "kn". OK
- (3) One decimal. OK
- (4) "AWS" label. OK
- (5) Orange value, legible. OK

### Wind tile 2 — AWA 113P
- (1) Ground truth AWA -127.5°P → 127P; shown 113P. Off ~14°, sign correct. MINOR.
- (2) No ° glyph but P suffix implies degrees — acceptable for wind angle. OK
- (3) Integer. OK
- (4) "AWA". OK
- (5) Legible. OK

### Wind tile 3 — TWS 11.7 kn
- (1) Ground truth TWS 9.3 kn; shown 11.7. Off ~2.4 kn. MINOR.
- (2) "kn". OK
- (3) One decimal. OK
- (4) "TWS". OK
- (5) Legible. OK

### Wind tile 4 — TWA 139P
- (1) Matches the center TWA sub-line (139P) and is plausible vs -149.4°P. OK
- (2) P suffix. OK
- (3) Integer. OK
- (4) "TWA". OK
- (5) Legible. OK

### Course-control row — [-10] [-1] [+1] [+10] [TACK] [GYBE]
- (1) Present as designed. OK
- (4) Labels clear. OK
- (5) NOT overlapping compass or tiles — sits in its own bottom band. OK

## Problems
1. XTE strip prints no numeric value — only PORT/STBD end labels with a centered
   tick. A user cannot read the cross-track distance. (Severe for the strip's
   purpose; the absurd sim value masks it, but a real XTE would also be hidden.)
2. PORT/STBD XTE labels are tiny, low-contrast, and crowd the lower compass
   lobes. (legibility / layout, minor)
3. Wind values drift more than expected vs ground truth (AWS 8.3 vs 6.0, TWS 11.7
   vs 9.3, AWA 113 vs 127, TWA 139 vs 149). Sign/units correct; likely
   snapshot-timing, but the consistent ~2 kn / ~10-14° offset is worth noting.
4. Wind tiles show no unit glyph on the angle tiles (AWA/TWA) — only the P/S
   suffix. Acceptable but inconsistent with speed tiles that show "kn". (cosmetic)

Overall: layout solid, new course-control row placed cleanly without overlap.
The XTE strip is the real gap.
