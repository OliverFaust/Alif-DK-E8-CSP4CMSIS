#!/usr/bin/env bash
# Fetch the PyTorch::ExecuTorch 1.1.0-rc1-build.12 CMSIS-Pack that neuropathway/ needs.
#
#   Source:  https://github.com/Arm-Examples/ModelNova, pinned commit
#            1826b9883e94ed6059f8fee11f9e787eb2c64a19 (the pack was removed from
#            ModelNova's main branch later, so a newer commit will not do)
#   Target:  <repo root>/tools/modelnova/packs/PyTorch.ExecuTorch.1.1.0-rc1-build.12
#            (the path Neuropathway.csolution.yml loads the pack from)
#   Licence: BSD 3-Clause, Copyright (c) Meta Platforms, Inc. and affiliates
#            (the pack's own LICENSE file)
#
# Only the pack directory is downloaded (partial clone + sparse checkout).
# Usage: scripts/fetch_executorch_pack.sh [--force]
#   --force  delete tools/modelnova/ and fetch again
set -euo pipefail

REPO_URL="https://github.com/Arm-Examples/ModelNova.git"
COMMIT="1826b9883e94ed6059f8fee11f9e787eb2c64a19"
PACK="PyTorch.ExecuTorch.1.1.0-rc1-build.12"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/tools/modelnova"
PACK_DIR="$DEST/packs/$PACK"
PDSC="$PACK_DIR/PyTorch.ExecuTorch.pdsc"

case "${1:-}" in
    "")      ;;
    --force) rm -rf "$DEST" ;;
    *)       echo "usage: $0 [--force]" >&2; exit 2 ;;
esac

if [[ -f "$PDSC" ]]; then
    echo "ExecuTorch pack already present: $PACK_DIR"
    exit 0
fi

if ! command -v git >/dev/null 2>&1; then
    echo "error: git is not installed (sudo apt install git)" >&2
    exit 1
fi

if [[ -e "$DEST" ]]; then
    echo "error: $DEST exists but does not contain the pack; rerun with --force to replace it" >&2
    exit 1
fi

echo "Fetching $PACK from $REPO_URL @ $COMMIT ..."
git init --quiet "$DEST"
git -C "$DEST" remote add origin "$REPO_URL"
git -C "$DEST" sparse-checkout set "packs/$PACK"
git -C "$DEST" fetch --quiet --depth 1 --filter=blob:none origin "$COMMIT"
git -C "$DEST" checkout --quiet FETCH_HEAD

if [[ ! -f "$PDSC" || ! -f "$PACK_DIR/LICENSE" ]]; then
    echo "error: fetch finished but $PDSC (or its LICENSE) is missing" >&2
    exit 1
fi

echo "ExecuTorch pack ready: $PACK_DIR"
echo "  source:  $REPO_URL"
echo "  commit:  $COMMIT"
echo "  licence: BSD 3-Clause, Meta Platforms, Inc. and affiliates (see $PACK_DIR/LICENSE)"
