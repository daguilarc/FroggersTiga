#!/usr/bin/env bash
# check_no_frozen_includes.sh -- a mechanical
# check, like the existing include guards, that no app TU includes from
# the frozen tree (src/): the DSP port is a sanctioned COPY,
# not a shared header, so no file under app/ may ever #include out of it,
# however the path is spelled (quoted, angle-bracket, with or
# without a leading ../).
#
# Usage: check_no_frozen_includes.sh <app-dir>

set -euo pipefail

app_dir="${1:?app directory required}"
frozen_pattern='^[[:space:]]*#[[:space:]]*include[[:space:]]*["<](\.\./)*src/'

status=0
while IFS= read -r -d '' file; do
  if grep -inE "$frozen_pattern" "$file" > /dev/null; then
    echo "check_no_frozen_includes: FAIL - $file includes from a frozen tree:" >&2
    grep -inE "$frozen_pattern" "$file" >&2
    status=1
  fi
done < <(find "$app_dir" -type f \( -name '*.hpp' -o -name '*.cpp' \) -not -path "$app_dir/build/*" -print0)

if [ "$status" -eq 0 ]; then
  echo "check_no_frozen_includes: OK - no app/** file includes from src/"
fi
exit "$status"
