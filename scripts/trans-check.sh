#!/bin/sh
#
# Validate a single .po catalog for release/commit readiness.
#
# Usage: scripts/trans-check.sh <po-file>
#
# Gates, each reported by name on failure:
#   1. msgfmt -c --check-format: parses cleanly, valid header, and no
#      format-specifier type losses (%d vs %s) or count drift.
#   2. zero fuzzy entries.
#   3. zero untranslated entries.
#
# Exit status: 0 all gates pass, 1 any gate fails, 2 usage error.

l="$1"

if [ -z "$l" ]; then
	echo "usage: $0 <po-file>" >&2
	exit 2
fi

if [ ! -f "$l" ]; then
	echo "$l: no such file" >&2
	exit 2
fi

rc=0

# Gate 1: syntax, header, and format-specifier checks.
if ! msgfmt -c --check-format -o /dev/null "$l"; then
	echo "$l: FAIL msgfmt -c --check-format reported errors"
	rc=1
fi

# Gates 2 and 3: derive fuzzy and untranslated counts from msgfmt statistics.
# A count word is absent from the summary when its category is empty, so
# fall back to zero when grep finds no match.
stats=$(msgfmt --statistics -o /dev/null "$l" 2>&1)
fuzzy=$(printf '%s\n' "$stats" | grep -oP '\d+(?= fuzzy)' || echo 0)
untranslated=$(printf '%s\n' "$stats" | grep -oP '\d+(?= untranslated)' || echo 0)

if [ "$fuzzy" -ne 0 ]; then
	echo "$l: FAIL $fuzzy fuzzy entries remain"
	rc=1
fi

if [ "$untranslated" -ne 0 ]; then
	echo "$l: FAIL $untranslated untranslated entries remain"
	rc=1
fi

if [ "$rc" -eq 0 ]; then
	echo "$l: OK complete and error-free"
fi

exit $rc
