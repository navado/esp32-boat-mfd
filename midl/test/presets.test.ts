import { test, expect } from "vitest";
import { expand, countTiles, depth } from "../src/presets";
import type { Node } from "../src/types";

test("'full' preset expands to a single leaf", () => {
  expect(expand({ preset: "full", slots: ["x"] })).toEqual({ element: "x" });
});

test("'hero-split' expands {1,{2,3}} to row[leaf, col[leaf,leaf]]", () => {
  expect(expand({ preset: "hero-split", slots: ["a", "b", "c"] })).toEqual({
    dir: "row",
    children: [{ element: "a" }, { dir: "col", children: [{ element: "b" }, { element: "c" }] }],
  });
});

test("unknown preset throws", () => {
  expect(() => expand({ preset: "nope", slots: [] })).toThrow(/unknown preset/);
});

test("wrong slot count throws", () => {
  expect(() => expand({ preset: "full", slots: [] })).toThrow(/slots/);
});

test("countTiles and depth on an expanded tree", () => {
  const t: Node = expand({ preset: "hero-split", slots: ["a", "b", "c"] });
  expect(countTiles(t)).toBe(3);
  expect(depth(t)).toBe(3);
});
