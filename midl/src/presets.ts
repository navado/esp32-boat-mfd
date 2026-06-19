import type { Node } from "./types";

export type PresetExpander = (slots: string[]) => Node;

function req(slots: string[], n: number): string[] {
  if (slots.length !== n) throw new Error(`preset expects ${n} slots, got ${slots.length}`);
  return slots;
}

// Registry of named presets. Each expands to a Node (which may itself
// contain further presets — expand() recurses). Add new presets here.
export const PRESETS: Record<string, PresetExpander> = {
  full: (s) => ({ element: req(s, 1)[0] }),
  "hero-split": (s) => {
    const [a, b, c] = req(s, 3);
    return { dir: "row", children: [{ element: a }, { dir: "col", children: [{ element: b }, { element: c }] }] };
  },
};

export function expand(n: Node): Node {
  if ("preset" in n) {
    const fn = PRESETS[n.preset];
    if (!fn) throw new Error(`unknown preset: ${n.preset}`);
    return expand(fn(n.slots ?? []));
  }
  if ("children" in n) return { ...n, children: n.children.map(expand) };
  if ("cells" in n) return { ...n, cells: n.cells.map(expand) };
  return n;
}

export function countTiles(n: Node): number {
  if ("element" in n) return 1;
  if ("children" in n) return n.children.reduce((a, c) => a + countTiles(c), 0);
  if ("cells" in n) return n.cells.reduce((a, c) => a + countTiles(c), 0);
  return 0; // a preset node (should not appear after expand)
}

export function depth(n: Node): number {
  if ("element" in n) return 1;
  const kids = "children" in n ? n.children : "cells" in n ? n.cells : [];
  return kids.length ? 1 + Math.max(...kids.map(depth)) : 1;
}
