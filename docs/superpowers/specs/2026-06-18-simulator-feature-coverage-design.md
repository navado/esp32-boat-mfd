# Simulator Feature-Coverage Enrichment (`fake_boat.py`)

**Status:** Queued (not yet started)
**Date:** 2026-06-18
**Repo:** firmware `espdisp` (this repo), file `tools/fake_boat.py`
**Related:** `2026-06-18-signalk-screens-preview-fidelity-design.md` — that rework makes the
previews faithful; this work generates the data that *exercises* them. Independent specs.

## Problem

`tools/fake_boat.py` (91 lines) pushes ~17 always-on sinusoidal paths to a SignalK server so
the dashboard has something to draw. It never emits the firmware's **usually-empty** render
paths — route/waypoint, autopilot, polar targets, depth-below-keel — nor any **AIS** traffic.
Those code paths (and the recently added marker rings, route screen, AP HUD, wind-steer) ship
untested against live data because nothing populates them.

The firmware already *subscribes* to all of these (`FULL_PATHS` in `src/signalk.cpp`) and the
parser already *handles* them (`src/signalk_parser.cpp`); only the data source is missing.

## Goal

Turn `fake_boat.py` into a **phase-driven simulator** that emits self-consistent, realistic
data for every renderable firmware feature, cycling the usually-empty states through both
populated and cleared phases so the empty/stale render paths are exercised too.

## Design

Stays a **single self-contained Python file** (no new deps beyond the existing
`websockets` — keeps `make demo-up` and the `MANAGER_DIR` invocation working). Same
WS-provider auth/connection (login → token → `subscribe=none` stream).

### Components (one file, focused units)

- **`geo`** — pure helpers: `dead_reckon(lat, lon, cog_rad, sog_ms, dt)`, `bearing(a, b)`,
  `distance(a, b)`, `cross_track(a, b, p)`. Great-circle math, ~30 lines.
- **`BoatState`** — integrates own position from SOG/COG each tick so all derived nav is
  self-consistent (XTE/BTW actually agree with position and COG, not independent sinusoids).
- **`Route`** — ordered waypoints (lat/lon). Computes to the active leg each tick:
  `crossTrackError`, `bearingTrackTrue` (CTS), `nextPoint.bearingTrue` (BTW),
  `nextPoint.distance` (DTW), `velocityMadeGood` (VMG = SOG·cos(BTW−COG)). Advances to the
  next waypoint when DTW < arrival threshold.
- **`AisFleet`** — N synthetic vessels, each dead-reckoned on its own course/speed; at least
  one held on a converging CPA track while present. Vessels spawn/despawn over time.
- **`PhaseScheduler`** — slow timers driving the usually-empty → populated → cleared cycle.

### Data emitted

Self context (`vessels.self`), in addition to today's 17 paths:

| Path | Source |
|---|---|
| `navigation.courseRhumbline.crossTrackError` | `Route` (derived) |
| `navigation.courseRhumbline.bearingTrackTrue` | `Route` |
| `navigation.courseRhumbline.nextPoint.bearingTrue` | `Route` |
| `navigation.courseRhumbline.nextPoint.distance` | `Route` |
| `navigation.courseRhumbline.velocityMadeGood` | `Route` |
| `steering.autopilot.state` | `PhaseScheduler` (`standby`/`auto`/`wind`) |
| `steering.autopilot.target.headingTrue` | `PhaseScheduler` (only when engaged) |
| `performance.beatAngle`, `performance.gybeAngle` | constant-ish, slight TWS variation |
| `environment.depth.belowKeel` | `belowTransducer − draft` |
| `navigation.closestApproach.bearingTrue` / `.distance` | bearing/range to nearest AIS target |

AIS: 3–4 full vessel contexts `vessels.urn:mrn:imo:mmsi:<MMSI>` — dynamic
(`navigation.position`, `speedOverGround`, `courseOverGroundTrue`, `headingTrue`) each tick;
static (`name`, `mmsi`, `design.aisShipType`) at a slower realistic cadence (~every 6 s).

> `navigation.closestApproach.*` is a **sim convention** (no fixed SignalK standard for an
> own-ship bearing-to-target). It is an angle-typed self path so a compass **marker ring** can
> bind to it and render an AIS bug on-device today (per the marker-rings design: deriving a
> bearing-to-target is a server/source concern; the firmware renders whatever angle path is
> bound). Per-screen subscribe picks it up when a screen with such a marker is active.

### Phase scheduling (the "cycle through phases" behavior)

- **Route:** `ACTIVE` (legs advance, all route paths emitted) → `ARRIVED` (route paths
  cleared — stop emitting so the device shows them empty/stale) → brief `INACTIVE` →
  re-`ACTIVATE`. Full cycle a few minutes.
- **Autopilot:** `STANDBY` (no target emitted) → `AUTO` (target = current heading, held) →
  `WIND` (target derived to hold the current TWA) → `STANDBY`.
- **AIS fleet:** target count breathes 0 → full → partial → 0 over time, exercising the
  empty-radar and populated-radar paths.
- **Always-on (once present):** polar targets, depthKeel, current/tide (existing).

### CLI

Keep positional `host port` (defaults `localhost 3000`). Add only:

- `--steady` — freeze phases fully populated (route active mid-leg, AP=`auto`, full AIS
  fleet) for screenshots / soak.
- `--dump` — print deltas as JSON lines to stdout without connecting (offline inspection +
  the smoke check below).
- `--ais N` — number of AIS targets (default 4).

## Testing & verification

No Python test harness exists; `make lint` runs `py_compile`. Verification plan:

1. `python3 -m py_compile tools/fake_boat.py` (matches `make lint`).
2. **Offline smoke check:** `python3 tools/fake_boat.py --dump --steady` and assert the
   output contains: all five route paths, `steering.autopilot.state == "auto"` with a
   target, `performance.beatAngle`, `environment.depth.belowKeel`,
   `navigation.closestApproach.bearingTrue`, and ≥1 `vessels.urn:…` context; plus an XTE
   sign assertion (boat offset to the right of track ⇒ positive XTE).
3. **Live:** `make demo-up`; confirm the device renders BTW/DTW, AP-engaged, route, and an
   AIS marker (when bound), and the manager live-view shows the same.

## Out of scope

- Firmware/manager changes — none needed (firmware already subscribes `FULL_PATHS`; AIS
  markers are user-configured against the published angle path).
- A second AIS consumer in firmware (other-vessel contexts are emitted for the SK server +
  manager + marker binding; the device has no AIS target list and none is added here).
- The screens preview-fidelity rework (separate spec).

## Notes for the implementer

- Keep it library-light and single-file; do not pull in a charting/AIS library for ~40 lines
  of great-circle math.
- AIS deltas are one context per message — send the self delta plus one delta per active
  vessel each tick (static fields throttled).
- `--dump` mode must exercise the exact same delta-building code as the live path (build the
  delta dict, then either `ws.send` or `print`), so the smoke check validates real output.
