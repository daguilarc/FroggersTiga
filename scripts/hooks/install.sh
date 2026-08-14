#!/usr/bin/env bash
# Install this repo's git hooks into .git/hooks.
#
# .git/hooks is not tracked, so hooks live in scripts/hooks/ (tracked) and are
# copied in. Re-run this after cloning, or after a hook changes.
#
# Existing hooks are preserved: an unrelated hook already in place is left
# alone unless it is an older copy of the same tracked hook.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/scripts/hooks"
DST="$(git -C "$ROOT" rev-parse --git-path hooks)"

for hook in "$SRC"/*; do
  name="$(basename "$hook")"
  case "$name" in
    install.sh) continue ;;
  esac

  target="$DST/$name"
  if [ -e "$target" ] && ! cmp -s "$hook" "$target"; then
    if grep -q "scripts/hooks/install.sh" "$target" 2>/dev/null; then
      echo "updating $name (older tracked version)"
    else
      echo "SKIP $name: a different hook is already installed at $target" >&2
      echo "     merge it by hand if you want both." >&2
      continue
    fi
  fi

  cp "$hook" "$target"
  chmod +x "$target"
  echo "installed $name"
done
