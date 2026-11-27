#!/usr/bin/env bash

set -euo pipefail

cd ~/dev/palettium

export TZ=UTC

python3 - <<'PY'
import os
import subprocess
import zipfile
from pathlib import Path

root = Path.cwd()
output = Path.home() / "Downloads" / "palsrc.zip"

output.parent.mkdir(parents=True, exist_ok=True)

# Include tracked files and non-ignored untracked files.
# Read actual working-tree contents, not committed versions.
raw = subprocess.check_output([
    "git", "ls-files",
    "--cached",
    "--others",
    "--exclude-standard",
    "-z"
])

files = sorted(set(
    os.fsdecode(name)
    for name in raw.split(b"\0")
    if name
))

included = 0
skipped = 0

with zipfile.ZipFile(
    output, "w",
    compression=zipfile.ZIP_DEFLATED,
    compresslevel=9
) as archive:

    for name in files:
        path = root / name

        # Skip deleted files and directories.
        if not path.is_file():
            skipped += 1
            continue

        if path.resolve() == output.resolve():
            continue

        archive.write(path, arcname=name)
        included += 1

with zipfile.ZipFile(output, "r") as archive:
    bad = archive.testzip()
    if bad is not None:
        raise RuntimeError("ZIP verification failed: " + bad)

print()
print("Archive:", output)
print("Files included:", included)
print("Missing/non-file entries skipped:", skipped)
print("Archive bytes:", output.stat().st_size)
print("ZIP integrity: PASS")
PY
