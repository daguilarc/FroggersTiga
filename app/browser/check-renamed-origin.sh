#!/usr/bin/env bash
# RENAMED-ORIGIN GATE. CI-runnable.
#
# The app-facing identity was renamed FroggersTiga -> frogg3rs (an app-wide
# tracked-name sweep). That is independent of, and MUST NOT regress
# because of, whatever the enclosing git repository/hosting name happens to
# be at any given moment. This gate greps the
# ASSEMBLED PACKAGE + CATALOG OUTPUT (the bytes that actually get
# published) for the retired app identity string.
#
# The original version
# used `grep -rIl`, whose `-I` flag SKIPS BINARY FILES ENTIRELY -- so it
# false-passed on the exact thing it exists to catch. A wasm binary can
# embed the literal string "FroggersTiga" via DWARF/compiler diagnostics
# carrying the build machine's absolute source paths, which is a build-path
# artifact rather than an app-identity defect. A naive "binary files must
# never contain FroggersTiga" rule would fail on such a build regardless of
# app-identity correctness, which is not the failure this gate is for. So
# the policy is split by what the match actually is:
#
#   1. TEXT files (grep -I, i.e. binaries excluded from this pass): ANY
#      "FroggersTiga" occurrence -> FAIL. Unchanged intent from before,
#      now actually reachable since text files were never the problem.
#   2. ALL files, binaries included (grep -a): an ORIGIN-URL FORM of the
#      old name (github.com/daguilarc/FroggersTiga,
#      daguilarc.github.io/FroggersTiga, /daguilarc/FroggersTiga) -> FAIL.
#      These are the actual publication hazard (a hostname/path a client
#      could resolve or a human could follow) and are never legitimately
#      present in build output, compiled or not.
#   3. Binary files containing the bare "FroggersTiga" string that is NOT
#      one of the origin-URL forms above (i.e. a compiled-in local
#      filesystem path, not a URL) -> WARN, non-fatal, printed per file.
#      These are exactly the build-path embeddings described above, absent
#      from any checkout whose path does not contain "FroggersTiga" -- which
#      now includes this machine's own working folder. A match here means the
#      artifact predates that, so it is a stale-build signal rather than an
#      app-identity defect: a loud warning, rather than lying green by
#      skipping the file or failing for a reason unrelated to app identity.
#   4. Empty-scan guard: if the target directory contains zero files, FAIL
#      -- a no-op scan (e.g. pointed at a nonexistent or emptied directory)
#      must never report OK.
set -euo pipefail

SITE_DIR="${1:-$(cd "$(dirname "$0")" && pwd)/dist/site}"
NEEDLE="FroggersTiga"
ORIGIN_PATTERNS=(
  -e "github.com/daguilarc/FroggersTiga"
  -e "daguilarc.github.io/FroggersTiga"
  -e "/daguilarc/FroggersTiga"
)

if [[ ! -d "$SITE_DIR" ]]; then
  echo "renamed-origin gate: $SITE_DIR does not exist -- run the package step first" >&2
  exit 1
fi

# 4. Empty-scan guard.
file_count=$(find "$SITE_DIR" -type f | wc -l | tr -d '[:space:]')
if [[ "$file_count" -eq 0 ]]; then
  echo "renamed-origin gate: FAIL -- $SITE_DIR contains no files (empty scan is not a pass)" >&2
  exit 1
fi

fail=0

# 1. Text files, any occurrence.
if text_matches=$(grep -rIl -- "$NEEDLE" "$SITE_DIR" 2>/dev/null); then
  echo "renamed-origin gate: FAIL -- found '$NEEDLE' in text file(s):" >&2
  echo "$text_matches" >&2
  fail=1
fi

# 2. All files (binaries included), origin-URL forms only.
if origin_matches=$(grep -rlaF "${ORIGIN_PATTERNS[@]}" -- "$SITE_DIR" 2>/dev/null); then
  echo "renamed-origin gate: FAIL -- found an old-origin URL form in:" >&2
  echo "$origin_matches" >&2
  fail=1
fi

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

# 3. Binary files with the bare string, not already caught above -> WARN.
# By construction (we only reach here after 1 and 2 both found nothing),
# any file matching this literal, all-files scan is a binary containing a
# non-origin-URL embedding of the old name.
if binary_matches=$(grep -rlaF -- "$NEEDLE" "$SITE_DIR" 2>/dev/null); then
  while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    echo "renamed-origin gate: WARN -- binary $file embeds '$NEEDLE' as a build-machine path (not a text file, not an origin-URL form); this artifact predates the current build path -- rebuild to clear it" >&2
  done <<< "$binary_matches"
fi

echo "renamed-origin gate: OK (no text-file or origin-URL '$NEEDLE' occurrences in $SITE_DIR)"
