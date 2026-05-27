#!/bin/bash
#
# Synchronize po/POTFILES.in with git-tracked source files and
# regenerate translation catalogs.
#
# Usage:
#   ./scripts/trans-update.sh          # regenerate POTFILES.in, make update-po, show stats
#   ./scripts/trans-update.sh --check  # report differences without modifying files
#

set -euo pipefail

cd "$(dirname "$0")/.."

POTFILES_IN="po/POTFILES.in"

check_only=false
case "${1:-}" in
    --check) check_only=true ;;
    "")      ;;
    *)
        echo "Usage: $(basename "$0") [--check]"
        echo ""
        echo "  --check   Report differences without modifying files"
        echo "  (none)    Regenerate POTFILES.in, run make update-po, show stats"
        exit 1
        ;;
esac

# All git-tracked C source, header, glade, and desktop entry files.
# xgettext keywords in po/Makevars control which strings are extracted;
# listing files without translatable strings is harmless.
scan_source_files() {
    git ls-files \
        'src/*.c' 'src/**/*.c' \
        'src/*.h' 'src/**/*.h' \
        'resources/*.glade' 'resources/**/*.glade' \
        'files/*.desktop.in' \
        | sort
}

# Compare scanned files against current POTFILES.in
check_potfiles() {
    local scanned current stale missing
    scanned=$(scan_source_files)
    current=$(grep -v '^#' "$POTFILES_IN" | grep -v '^[[:space:]]*$' | sort)

    stale=$(comm -23 <(echo "$current") <(echo "$scanned"))
    missing=$(comm -13 <(echo "$current") <(echo "$scanned"))

    local rc=0

    if [ -n "$stale" ]; then
        echo "Stale entries in $POTFILES_IN (no longer git-tracked):"
        echo "$stale" | while IFS= read -r f; do echo "  - $f"; done
        rc=1
    fi

    if [ -n "$missing" ]; then
        echo "Missing from $POTFILES_IN (git-tracked but not listed):"
        echo "$missing" | while IFS= read -r f; do echo "  + $f"; done
        rc=1
    fi

    if [ "$rc" -eq 0 ]; then
        echo "$POTFILES_IN is up to date."
    fi

    return $rc
}

# Regenerate POTFILES.in from git ls-files
regenerate_potfiles() {
    {
        echo "# List of source files containing translatable strings."
        scan_source_files
    } > "$POTFILES_IN"
    echo "Regenerated $POTFILES_IN"
}

# ---------------------------------------------------------------------------

if $check_only; then
    check_potfiles
    exit $?
fi

if ! check_potfiles; then
    echo ""
    echo "Regenerating $POTFILES_IN..."
    regenerate_potfiles
    echo ""
fi

echo "Running make update-po..."
make update-po
echo ""

# Display translation statistics
"$(dirname "$0")/trans-stats.sh"
