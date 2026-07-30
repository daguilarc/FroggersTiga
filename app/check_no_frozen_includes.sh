#!/usr/bin/env bash
# check_no_frozen_includes.sh -- tasks.md 3 preamble: "Add a mechanical
# check, like the existing include guards, that no app TU includes from
# those trees" (src/, sim/, desktop-v2/). Also covers the other frozen
# trees named in this packet's hard constraints (desktop/, wasm/, vcv/,
# web/) for the same reason: the DSP port is a sanctioned COPY (design D3),
# not a shared header, so no file under app/ may ever #include out of any
# of them, however the path is spelled (quoted, angle-bracket, with or
# without a leading ../).
#
# Usage: check_no_frozen_includes.sh <app-dir>

set -euo pipefail

app_dir="${1:?app directory required}"
frozen_pattern='^[[:space:]]*#[[:space:]]*include[[:space:]]*["<](\.\./)*(src|sim|desktop-v2|desktop|wasm|vcv|web)/'

status=0
while IFS= read -r -d '' file; do
  if grep -inE "$frozen_pattern" "$file" > /dev/null; then
    echo "check_no_frozen_includes: FAIL - $file includes from a frozen tree:" >&2
    grep -inE "$frozen_pattern" "$file" >&2
    status=1
  fi
done < <(find "$app_dir" -type f \( -name '*.hpp' -o -name '*.cpp' \) -not -path "$app_dir/build/*" -print0)

if [ "$status" -eq 0 ]; then
  echo "check_no_frozen_includes: OK - no app/** file includes from src/, sim/, desktop-v2/, desktop/, wasm/, vcv/, or web/"
fi
exit "$status"
