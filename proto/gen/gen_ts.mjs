// Run from proto/js: cd proto/js && node ../gen/gen_ts.mjs
// Uses createRequire so node resolves json-schema-to-typescript from proto/js/node_modules
// regardless of which directory this script lives in.
import { createRequire } from "node:module";
import { writeFileSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

const jsDir = new URL("../js/", import.meta.url);

// Resolve the library from proto/js where it is installed
const require = createRequire(new URL("package.json", jsDir));
const { compile } = require("json-schema-to-typescript");

const schemaPath = fileURLToPath(new URL("../schema/espdisp-control-1.schema.json", import.meta.url));
const outPath = fileURLToPath(new URL("../js/types.d.ts", import.meta.url));

const schema = JSON.parse(readFileSync(schemaPath, "utf8"));

// The root schema only declares `$defs` (no top-level type/properties), so
// json-schema-to-typescript prunes everything by default. Build a synthetic
// root that $refs every def, forcing each to be emitted as a named interface.
const root = {
  $id: schema.$id,
  title: "Espdisp",
  type: "object",
  properties: Object.fromEntries(Object.keys(schema.$defs).map((k) => [k, { $ref: "#/$defs/" + k }])),
  $defs: schema.$defs,
};

const ts = await compile(root, "Espdisp", {
  declareExternallyReferenced: true,
  additionalProperties: true, // forward-compat: allow unknown fields
  bannerComment: "// GENERATED from espdisp-control-1.schema.json — do not edit.",
});

writeFileSync(outPath, ts);
console.log("generated proto/js/types.d.ts");
