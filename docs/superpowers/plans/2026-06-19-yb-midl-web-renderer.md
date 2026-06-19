# YB-MIDL Web Renderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax.

**Goal:** A vanilla-TS + Canvas web renderer in the MIDL repo (`midl/web/`) that takes a YB-MIDL config + a device capability manifest, validates + resolves it for a chosen resolution class, solves the layout grammar into pixel rects, and paints the element catalog — a multi-resolution preview surface.

**Architecture:** Pure layout solver (split/grid → rects) and a preview pipeline (validate → resolve variant → expand presets → solve) are unit-tested with Vitest, importing `@yey-boats/midl` (the `ts/` package) via a Vite source alias. Canvas painters and a demo page are build- and screenshot-verified (not unit-tested, since they're visual). Plan 3 of 6.

**Tech Stack:** Vanilla TypeScript, Vite (dev/build), Vitest (tests), HTML Canvas 2D. Depends on `@yey-boats/midl`.

**Location:** `midl/web/` in the MIDL repo (`/Users/borissorochkin/code/embedded/midl`). Plans live in the espdisp hub; implementation is in the MIDL repo.

---

## File structure

```
midl/
  package.json              # add "web" to workspaces
  web/
    package.json
    tsconfig.json
    vite.config.ts          # resolve alias @yey-boats/midl -> ../ts/src/index.ts; vitest config
    index.html              # demo page
    src/
      solve.ts              # layout grammar -> pixel rects (pure)
      preview.ts            # validate + resolve + expand + solve pipeline
      paint.ts              # canvas painters per element type
      main.ts               # demo wiring (load config+manifest, render, class selector)
    test/
      solve.test.ts
      preview.test.ts
      fixtures/sample.config.yaml
```

---

## Task 1: Scaffold the web package

**Files:**
- Modify: `midl/package.json` (add `"web"` to workspaces)
- Create: `midl/web/package.json`, `midl/web/tsconfig.json`, `midl/web/vite.config.ts`, `midl/web/src/main.ts` (stub), `midl/web/index.html`, `midl/web/test/smoke.test.ts`

- [ ] **Step 1: Add `"web"` to the root workspaces in `midl/package.json`**

Change `"workspaces": ["ts"]` to `"workspaces": ["ts", "web"]`.

- [ ] **Step 2: Create `midl/web/package.json`**

```json
{
  "name": "@yey-boats/midl-web",
  "version": "0.1.0",
  "license": "PolyForm-Noncommercial-1.0.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vite build",
    "test": "vitest run"
  },
  "dependencies": {
    "@yey-boats/midl": "*"
  },
  "devDependencies": {
    "typescript": "^5.4.5",
    "vite": "^5.3.1",
    "vitest": "^1.6.0"
  }
}
```

- [ ] **Step 3: Create `midl/web/tsconfig.json`**

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "ES2022",
    "moduleResolution": "Bundler",
    "strict": true,
    "lib": ["ES2022", "DOM"],
    "types": [],
    "skipLibCheck": true,
    "noEmit": true
  },
  "include": ["src", "test", "vite.config.ts"]
}
```

- [ ] **Step 4: Create `midl/web/vite.config.ts`** (alias makes both Vite and Vitest import the MIDL TS source directly — no build of `ts/` needed)

```typescript
import { defineConfig } from "vite";
import { fileURLToPath } from "node:url";

export default defineConfig({
  resolve: {
    alias: {
      "@yey-boats/midl": fileURLToPath(new URL("../ts/src/index.ts", import.meta.url)),
    },
  },
  test: {
    environment: "node",
  },
});
```

- [ ] **Step 5: Create `midl/web/index.html`**

```html
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <title>YB-MIDL Preview</title>
    <style>
      body { margin: 0; background: #0b0f14; color: #cdd6e0; font-family: system-ui, sans-serif; }
      #bar { padding: 8px 12px; }
      canvas { display: block; margin: 12px auto; background: #000; border: 1px solid #243; }
      #issues { color: #f6c; white-space: pre-wrap; padding: 0 12px; }
    </style>
  </head>
  <body>
    <div id="bar">
      <label>Class:
        <select id="cls">
          <option value="square-480">square-480 (480x480)</option>
          <option value="landscape-800x480">landscape-800x480</option>
          <option value="landscape-1024x600">landscape-1024x600</option>
        </select>
      </label>
    </div>
    <canvas id="cv" width="480" height="480"></canvas>
    <div id="issues"></div>
    <script type="module" src="/src/main.ts"></script>
  </body>
</html>
```

- [ ] **Step 6: Create `midl/web/src/main.ts` (temporary stub)**

```typescript
import { validateDocument } from "@yey-boats/midl";
// Smoke import to confirm the alias resolves; real wiring lands in Task 4.
export const ready = typeof validateDocument === "function";
```

- [ ] **Step 7: Create `midl/web/test/smoke.test.ts`**

```typescript
import { test, expect } from "vitest";
import { validateDocument, expand } from "@yey-boats/midl";

test("can import @yey-boats/midl via the vite alias", () => {
  expect(typeof validateDocument).toBe("function");
  expect(expand({ preset: "full", slots: ["x"] })).toEqual({ element: "x" });
});
```

- [ ] **Step 8: Install and run**

Run: `cd /Users/borissorochkin/code/embedded/midl && npm install`
Then: `cd web && npx vitest run`
Expected: smoke test PASSES (the alias resolves `@yey-boats/midl` to the source).

- [ ] **Step 9: Commit (do NOT push)**

```bash
cd /Users/borissorochkin/code/embedded/midl
git add package.json web/
git commit -m "feat(web): scaffold @yey-boats/midl-web (vite + vitest + alias)"
```

---

## Task 2: Layout solver (grammar → pixel rects)

**Files:**
- Create: `midl/web/src/solve.ts`
- Test: `midl/web/test/solve.test.ts`

- [ ] **Step 1: Write the failing test**

```typescript
// midl/web/test/solve.test.ts
import { test, expect } from "vitest";
import { solveLayout, type Rect } from "../src/solve";

const vp: Rect = { x: 0, y: 0, w: 480, h: 480 };

test("a leaf fills the whole rect", () => {
  expect(solveLayout({ element: "a" }, vp)).toEqual([{ elementId: "a", rect: vp }]);
});

test("a row split halves the width", () => {
  const out = solveLayout({ dir: "row", children: [{ element: "a" }, { element: "b" }] }, vp);
  expect(out).toEqual([
    { elementId: "a", rect: { x: 0, y: 0, w: 240, h: 480 } },
    { elementId: "b", rect: { x: 240, y: 0, w: 240, h: 480 } },
  ]);
});

test("a col split halves the height", () => {
  const out = solveLayout({ dir: "col", children: [{ element: "a" }, { element: "b" }] }, vp);
  expect(out.map((p) => p.rect.h)).toEqual([240, 240]);
  expect(out[1].rect.y).toBe(240);
});

test("weights distribute proportionally", () => {
  const out = solveLayout({ dir: "row", children: [{ element: "a" }, { element: "b" }], weights: [1, 3] }, vp);
  expect(out[0].rect.w).toBe(120);
  expect(out[1].rect.w).toBe(360);
  expect(out[1].rect.x).toBe(120);
});

test("a 2x2 grid yields four equal cells", () => {
  const out = solveLayout(
    { rows: 2, cols: 2, cells: [{ element: "a" }, { element: "b" }, { element: "c" }, { element: "d" }] },
    vp,
  );
  expect(out.map((p) => p.elementId)).toEqual(["a", "b", "c", "d"]);
  expect(out[3].rect).toEqual({ x: 240, y: 240, w: 240, h: 240 });
});

test("nested {1,{2,3}} places three leaves without overlap", () => {
  const out = solveLayout(
    { dir: "row", children: [{ element: "hero" }, { dir: "col", children: [{ element: "b" }, { element: "c" }] }] },
    vp,
  );
  expect(out.map((p) => p.elementId)).toEqual(["hero", "b", "c"]);
  expect(out[0].rect.w).toBe(240);
  expect(out[1].rect).toEqual({ x: 240, y: 0, w: 240, h: 240 });
  expect(out[2].rect).toEqual({ x: 240, y: 240, w: 240, h: 240 });
});
```

- [ ] **Step 2: Run, confirm FAIL**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run test/solve.test.ts`
Expected: FAIL — Cannot find module '../src/solve'.

- [ ] **Step 3: Create `midl/web/src/solve.ts`**

```typescript
import type { Node } from "@yey-boats/midl";

export interface Rect { x: number; y: number; w: number; h: number; }
export interface Placement { elementId: string; rect: Rect; }

// Walk an EXPANDED layout tree (no preset nodes) and assign a pixel rect
// to every leaf. Splits divide along their axis by weight (default equal);
// grids divide into rows x cols equal cells, row-major.
export function solveLayout(node: Node, rect: Rect): Placement[] {
  if ("element" in node) return [{ elementId: node.element, rect }];

  if ("children" in node) {
    const weights = node.weights ?? node.children.map(() => 1);
    const total = weights.reduce((a, b) => a + b, 0);
    const out: Placement[] = [];
    let off = 0;
    node.children.forEach((child, i) => {
      const frac = weights[i] / total;
      let childRect: Rect;
      if (node.dir === "row") {
        const w = rect.w * frac;
        childRect = { x: rect.x + off, y: rect.y, w, h: rect.h };
        off += w;
      } else {
        const h = rect.h * frac;
        childRect = { x: rect.x, y: rect.y + off, w: rect.w, h };
        off += h;
      }
      out.push(...solveLayout(child, childRect));
    });
    return out;
  }

  if ("cells" in node) {
    const cw = rect.w / node.cols;
    const ch = rect.h / node.rows;
    const out: Placement[] = [];
    node.cells.forEach((child, i) => {
      const r = Math.floor(i / node.cols);
      const c = i % node.cols;
      out.push(...solveLayout(child, { x: rect.x + c * cw, y: rect.y + r * ch, w: cw, h: ch }));
    });
    return out;
  }

  return []; // preset node — must be expanded before solving
}
```

- [ ] **Step 4: Run, confirm PASS**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run test/solve.test.ts`
Expected: PASS (6 tests).

- [ ] **Step 5: Commit**

```bash
cd /Users/borissorochkin/code/embedded/midl
git add web/src/solve.ts web/test/solve.test.ts
git commit -m "feat(web): layout-grammar pixel-rect solver"
```

---

## Task 3: Preview pipeline (validate → resolve → expand → solve)

**Files:**
- Create: `midl/web/src/preview.ts`
- Create: `midl/web/test/fixtures/sample.config.yaml`
- Test: `midl/web/test/preview.test.ts`

- [ ] **Step 1: Create the fixture `midl/web/test/fixtures/sample.config.yaml`**

```yaml
midl: 1.0.0
screens:
  - id: dash
    elements:
      wind: { type: windrose, bindings: { value: { kind: signalk, path: environment.wind.speedApparent } } }
      sog: { type: single-value, bindings: { value: { kind: signalk, path: navigation.speedOverGround } } }
      depth: { type: single-value, bindings: { value: { kind: signalk, path: environment.depth.belowTransducer } } }
    layout:
      dir: row
      children:
        - element: wind
        - dir: col
          children: [{ element: sog }, { element: depth }]
```

- [ ] **Step 2: Write the failing test**

```typescript
// midl/web/test/preview.test.ts
import { test, expect } from "vitest";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { previewConfig } from "../src/preview";
import type { Manifest } from "@yey-boats/midl";

const here = dirname(fileURLToPath(import.meta.url));
const sample = readFileSync(join(here, "fixtures", "sample.config.yaml"), "utf8");

const manifest: Manifest = {
  midl: "1.0.0",
  board: "sunton-4848s040",
  classes: [{ id: "square-480", maxTiles: 4, maxDepth: 3, elements: ["windrose", "single-value"] }],
  elements: [
    { type: "windrose", bindings: ["value"] },
    { type: "single-value", bindings: ["value"] },
  ],
  sources: ["signalk"],
};
const vp = { x: 0, y: 0, w: 480, h: 480 };

test("a valid config previews with placed elements", () => {
  const r = previewConfig(sample, manifest, "square-480", vp);
  expect(r.ok).toBe(true);
  expect(r.screens).toHaveLength(1);
  const ids = r.screens[0].placements.map((p) => p.elementId).sort();
  expect(ids).toEqual(["depth", "sog", "wind"]);
  // every placement is within the viewport
  for (const p of r.screens[0].placements) {
    expect(p.rect.x).toBeGreaterThanOrEqual(0);
    expect(p.rect.x + p.rect.w).toBeLessThanOrEqual(480.0001);
  }
});

test("element defs are exposed for painting", () => {
  const r = previewConfig(sample, manifest, "square-480", vp);
  expect(r.elements["dash"]["wind"].type).toBe("windrose");
});

test("an unsatisfiable config returns issues, not placements", () => {
  const bad = manifest.classes[0].elements = ["single-value"]; // windrose no longer supported
  void bad;
  const r = previewConfig(sample, manifest, "square-480", vp);
  expect(r.ok).toBe(false);
  expect(r.issues.some((i) => /not supported/.test(i.message))).toBe(true);
  expect(r.screens).toEqual([]);
});
```

- [ ] **Step 3: Run, confirm FAIL**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run test/preview.test.ts`
Expected: FAIL — Cannot find module '../src/preview'.

- [ ] **Step 4: Create `midl/web/src/preview.ts`**

```typescript
import { validateDocument, parseDoc, expand } from "@yey-boats/midl";
import type { ConfigDoc, Element, Manifest, Node } from "@yey-boats/midl";
import { solveLayout, type Placement, type Rect } from "./solve";

export interface ScreenPlan { screenId: string; placements: Placement[]; }
export interface PreviewResult {
  ok: boolean;
  issues: { path: string; message: string }[];
  screens: ScreenPlan[];
  elements: Record<string, Record<string, Element>>; // screenId -> elementId -> element
}

// Validate the document against the target manifest/class, then for each
// screen pick the class variant (or base), expand presets, and solve rects.
export function previewConfig(text: string, manifest: Manifest, className: string, viewport: Rect): PreviewResult {
  const res = validateDocument(text, manifest, className);
  if (!res.ok) return { ok: false, issues: res.issues, screens: [], elements: {} };

  const doc = parseDoc(text) as ConfigDoc;
  const screens: ScreenPlan[] = [];
  const elements: Record<string, Record<string, Element>> = {};
  for (const screen of doc.screens) {
    const variant = screen.variants?.find((v) => v.class === className);
    const layout: Node = variant ? variant.layout : screen.layout;
    const tree = expand(layout);
    screens.push({ screenId: screen.id, placements: solveLayout(tree, viewport) });
    elements[screen.id] = screen.elements;
  }
  return { ok: true, issues: [], screens, elements };
}
```

- [ ] **Step 5: Run, confirm PASS**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run test/preview.test.ts`
Expected: PASS (3 tests).

- [ ] **Step 6: Full web suite + commit**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run` (all green)

```bash
cd /Users/borissorochkin/code/embedded/midl
git add web/src/preview.ts web/test/preview.test.ts web/test/fixtures/
git commit -m "feat(web): preview pipeline (validate, resolve, expand, solve)"
```

---

## Task 4: Canvas painters + demo wiring (build + screenshot verified)

Painters are visual, so they are verified by a successful Vite build and a Playwright screenshot rather than unit tests.

**Files:**
- Create: `midl/web/src/paint.ts`
- Modify: `midl/web/src/main.ts` (replace the stub)

- [ ] **Step 1: Create `midl/web/src/paint.ts`**

```typescript
import type { Element } from "@yey-boats/midl";
import type { Placement } from "./solve";

type Ctx = CanvasRenderingContext2D;

// Sample value provider for the preview (no live SignalK in the browser yet).
export type ValueFn = (el: Element) => string;

const COLORS = { panel: "#11161d", edge: "#2a3a4a", fg: "#dce6f0", dim: "#7b8aa0", accent: "#3fa7ff", warn: "#ffb547" };

function frame(ctx: Ctx, p: Placement, title: string): void {
  const { x, y, w, h } = p.rect;
  ctx.fillStyle = COLORS.panel;
  ctx.fillRect(x + 2, y + 2, w - 4, h - 4);
  ctx.strokeStyle = COLORS.edge;
  ctx.strokeRect(x + 2, y + 2, w - 4, h - 4);
  ctx.fillStyle = COLORS.dim;
  ctx.font = "12px system-ui";
  ctx.textAlign = "left";
  ctx.fillText(title.toUpperCase(), x + 10, y + 18);
}

function centerText(ctx: Ctx, p: Placement, text: string, size: number, color: string): void {
  const { x, y, w, h } = p.rect;
  ctx.fillStyle = color;
  ctx.font = `${size}px system-ui`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(text, x + w / 2, y + h / 2 + 6);
  ctx.textBaseline = "alphabetic";
}

function dial(ctx: Ctx, p: Placement, value: string, ringColor: string): void {
  const { x, y, w, h } = p.rect;
  const cx = x + w / 2, cy = y + h / 2 + 6, r = Math.min(w, h) * 0.32;
  ctx.strokeStyle = ringColor;
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.stroke();
  ctx.lineWidth = 1;
  centerText(ctx, p, value, 26, COLORS.fg);
}

function bar(ctx: Ctx, p: Placement, frac: number): void {
  const { x, y, w, h } = p.rect;
  const bx = x + 14, bw = w - 28, by = y + h / 2, bh = 14;
  ctx.strokeStyle = COLORS.edge;
  ctx.strokeRect(bx, by, bw, bh);
  ctx.fillStyle = COLORS.accent;
  ctx.fillRect(bx, by, Math.max(0, Math.min(1, frac)) * bw, bh);
}

// Paint one screen's placements. `valueFn` supplies display strings.
export function paintScreen(
  ctx: Ctx,
  placements: Placement[],
  elements: Record<string, Element>,
  valueFn: ValueFn,
): void {
  for (const p of placements) {
    const el = elements[p.elementId];
    if (!el) continue;
    const title = el.name ?? p.elementId;
    frame(ctx, p, title);
    const val = valueFn(el);
    switch (el.type) {
      case "compass": dial(ctx, p, val, COLORS.accent); break;
      case "windrose": dial(ctx, p, val, COLORS.warn); break;
      case "gauge": dial(ctx, p, val, COLORS.accent); break;
      case "bar": bar(ctx, p, 0.6); break;
      case "button": centerText(ctx, p, title, 18, COLORS.accent); break;
      default: centerText(ctx, p, val, 34, COLORS.fg); break; // single-value, text, trend, autopilot
    }
  }
}
```

- [ ] **Step 2: Replace `midl/web/src/main.ts`**

```typescript
import { previewConfig } from "./preview";
import { paintScreen, type ValueFn } from "./paint";
import type { Manifest } from "@yey-boats/midl";

const SAMPLE = `midl: 1.0.0
screens:
  - id: dash
    elements:
      wind: { type: windrose, name: WIND, bindings: { value: { kind: signalk, path: environment.wind.speedApparent } } }
      sog: { type: single-value, name: SOG, bindings: { value: { kind: signalk, path: navigation.speedOverGround } } }
      depth: { type: single-value, name: DEPTH, bindings: { value: { kind: signalk, path: environment.depth.belowTransducer } } }
      batt: { type: bar, name: BATT, bindings: { value: { kind: signalk, path: electrical.batteries.house.stateOfCharge } } }
    layout:
      rows: 2
      cols: 2
      cells: [{ element: wind }, { element: sog }, { element: depth }, { element: batt }]
`;

const MANIFEST: Manifest = {
  midl: "1.0.0",
  board: "preview",
  classes: [
    { id: "square-480", width: 480, height: 480, maxTiles: 4, maxDepth: 3, elements: ["windrose", "single-value", "bar"] },
    { id: "landscape-800x480", width: 800, height: 480, maxTiles: 6, maxDepth: 3, elements: ["windrose", "single-value", "bar"] },
    { id: "landscape-1024x600", width: 1024, height: 600, maxTiles: 6, maxDepth: 4, elements: ["windrose", "single-value", "bar"] },
  ],
  elements: [
    { type: "windrose", bindings: ["value"] },
    { type: "single-value", bindings: ["value"] },
    { type: "bar", bindings: ["value"] },
  ],
  sources: ["signalk"],
};

const SAMPLE_VALUES: Record<string, string> = {
  windrose: "12.4", "single-value": "6.1", bar: "78%",
};
const valueFn: ValueFn = (el) => SAMPLE_VALUES[el.type] ?? "--";

function render(className: string): void {
  const cls = MANIFEST.classes.find((c) => c.id === className)!;
  const canvas = document.getElementById("cv") as HTMLCanvasElement;
  canvas.width = cls.width!;
  canvas.height = cls.height!;
  const ctx = canvas.getContext("2d")!;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  const res = previewConfig(SAMPLE, MANIFEST, className, { x: 0, y: 0, w: canvas.width, h: canvas.height });
  const issues = document.getElementById("issues")!;
  if (!res.ok) { issues.textContent = res.issues.map((i) => `${i.path}: ${i.message}`).join("\n"); return; }
  issues.textContent = "";
  for (const sp of res.screens) paintScreen(ctx, sp.placements, res.elements[sp.screenId], valueFn);
}

const sel = document.getElementById("cls") as HTMLSelectElement;
sel.addEventListener("change", () => render(sel.value));
render(sel.value);
```

- [ ] **Step 2.5: Type-check + build**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx tsc --noEmit -p tsconfig.json && npx vite build`
Expected: tsc exit 0; Vite build succeeds, emits `dist/`.

- [ ] **Step 3: Screenshot-verify the demo with Playwright (webapp-testing skill)**

Start the dev server in the background and capture a screenshot of the rendered canvas:
- Run `cd /Users/borissorochkin/code/embedded/midl/web && npx vite --port 5180` in the background.
- Use the Playwright MCP browser tools: navigate to `http://localhost:5180`, wait for the canvas, take a screenshot.
- Confirm visually: four framed tiles (WIND dial, SOG value, DEPTH value, BATT bar) on a 480×480 canvas; switching the class selector to `landscape-1024x600` resizes the canvas and re-lays-out.
- Save the screenshot to `midl/web/preview-square-480.png` and stop the dev server.

- [ ] **Step 4: Confirm no regressions + commit**

Run: `cd /Users/borissorochkin/code/embedded/midl/web && npx vitest run` (all green)

```bash
cd /Users/borissorochkin/code/embedded/midl
git add web/src/paint.ts web/src/main.ts web/preview-square-480.png
git commit -m "feat(web): canvas element painters + multi-resolution demo page"
```

---

## Done criteria

- `cd midl/web && npx vitest run` is green (smoke + solver + pipeline).
- `npx tsc --noEmit` and `npx vite build` succeed.
- The demo page renders a 4-tile dashboard and re-lays-out across resolution classes; a screenshot is committed.
- The renderer imports `@yey-boats/midl` for all validation/resolution (no duplicated language logic).

## Notes for follow-on plans

- **Live data:** `valueFn` is a static stub; a later step wires a SignalK WebSocket (or replays `sk::Data`) so the preview animates. Keep `paintScreen` data-driven.
- **Painter fidelity:** painters are schematic ("preview at the moment"). Raising fidelity toward the device LVGL look (markers, arcs, fonts) is incremental and shares the catalog's element set.
- **Device-served bundle (spec §3.6):** the same `@yey-boats/midl` validator + a trimmed renderer can be built into the static bundle the device serves for plugin-less authoring (Plan 4/6).
- **Manifest source:** the demo hardcodes a manifest; wire it to load `schemas/gen/*.json` (Plan 4 manager) or `/api/diag` from a live device.
```
