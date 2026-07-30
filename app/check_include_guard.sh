#!/usr/bin/env bash
# check_include_guard.sh — tasks.md 1.3.
#
# Fails if any -I flag among $1 (a whitespace-separated CPPFLAGS-style
# string) resolves into the frozen, vendored Sheaf copy at
# desktop-v2/External/Sheaf ($2, an absolute path). That tree is a plain
# vendored directory kept only because desktop-v2 is frozen (design D1
# consequence) — this app must only ever resolve Sheaf headers through the
# External/Sheaf submodule.
#
# Usage: check_include_guard.sh "<cppflags>" "<frozen-vendored-sheaf-abspath>"

set -euo pipefail

cppflags="${1:-}"
frozen_dir="${2:?frozen vendored Sheaf path required}"

status=0
for flag in $cppflags; do
  case "$flag" in
    -I*)
      path="${flag#-I}"
      if [ -d "$path" ]; then
        resolved="$(cd "$path" && pwd -P)"
      else
        resolved="$path"
      fi
      case "$resolved" in
        "$frozen_dir"|"$frozen_dir"/*)
          echo "check_include_guard: FAIL - include path '$path' resolves into frozen vendored Sheaf copy ($frozen_dir)" >&2
          status=1
          ;;
      esac
      ;;
  esac
done

if [ "$status" -eq 0 ]; then
  echo "check_include_guard: OK - no -I resolves into $frozen_dir"
fi
exit "$status"
