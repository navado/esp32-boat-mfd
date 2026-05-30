Import("env")

import os
import re
import subprocess
from pathlib import Path


ROOT = Path(env["PROJECT_DIR"])
VERSION_FILE = ROOT / "VERSION"
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$")


def read_version():
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not SEMVER_RE.match(version):
        raise ValueError(f"invalid VERSION '{version}'")
    return version


def git_output(args, fallback):
    try:
        return subprocess.check_output(
            ["git", *args],
            cwd=str(ROOT),
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip() or fallback
    except Exception:
        return fallback


version = os.environ.get("ESPDISP_VERSION", read_version()).strip()
if not SEMVER_RE.match(version):
    raise ValueError(f"invalid ESPDISP_VERSION '{version}'")

commit = os.environ.get("GITHUB_SHA") or git_output(["rev-parse", "--short=12", "HEAD"], "unknown")
if len(commit) > 12 and re.fullmatch(r"[0-9a-fA-F]+", commit):
    commit = commit[:12]

env.Append(
    CPPDEFINES=[
        ("FW_NAME", env.StringifyMacro("espdisp")),
        ("FW_VERSION", env.StringifyMacro(version)),
        ("FW_GIT_COMMIT", env.StringifyMacro(commit)),
        ("PIO_ENV", env.StringifyMacro(env["PIOENV"])),
    ]
)

print(f"espdisp version: {version} ({commit}) env={env['PIOENV']}")
