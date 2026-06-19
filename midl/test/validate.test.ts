import { test, expect } from "vitest";
import { validateConfigStructure, validateManifestStructure } from "../src/validate";

const goodConfig = {
  midl: "1.0.0",
  screens: [{ id: "dash", elements: { sog: { type: "single-value" } }, layout: { element: "sog" } }],
};
const goodManifest = {
  midl: "1.0.0",
  board: "sunton-4848s040",
  classes: [{ id: "sunton-480", maxTiles: 4, maxDepth: 3 }],
  elements: [{ type: "single-value" }],
};

test("valid config has no structural issues", () => {
  expect(validateConfigStructure(goodConfig)).toEqual([]);
});

test("config missing 'midl' is rejected", () => {
  const bad = { screens: [] };
  const issues = validateConfigStructure(bad);
  expect(issues.length).toBeGreaterThan(0);
});

test("config with a malformed layout node is rejected", () => {
  const bad = { midl: "1.0.0", screens: [{ id: "d", elements: {}, layout: { bogus: true } }] };
  expect(validateConfigStructure(bad).length).toBeGreaterThan(0);
});

test("valid manifest has no structural issues", () => {
  expect(validateManifestStructure(goodManifest)).toEqual([]);
});
