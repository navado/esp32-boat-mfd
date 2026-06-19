// Use the 2020-12 dialect build of Ajv: the schemas use $defs / oneOf and
// declare $schema draft 2020-12. The default Ajv export is draft-07.
import Ajv2020 from "ajv/dist/2020";
import type { ValidateFunction } from "ajv";
import configSchema from "../schemas/yb-midl-config.schema.json";
import capsSchema from "../schemas/yb-midl-capabilities.schema.json";
import type { Issue } from "./types";

const ajv = new Ajv2020({ allErrors: true, strict: false });
// Cast: the imported JSON's inferred literal type does not match Ajv's
// AnySchemaObject; the runtime value is a valid schema object.
const vConfig: ValidateFunction = ajv.compile(configSchema as object);
const vCaps: ValidateFunction = ajv.compile(capsSchema as object);

function toIssues(v: ValidateFunction): Issue[] {
  return (v.errors ?? []).map((e) => ({ path: e.instancePath || "/", message: e.message ?? "invalid" }));
}

export function validateConfigStructure(doc: unknown): Issue[] {
  return vConfig(doc) ? [] : toIssues(vConfig);
}

export function validateManifestStructure(doc: unknown): Issue[] {
  return vCaps(doc) ? [] : toIssues(vCaps);
}
