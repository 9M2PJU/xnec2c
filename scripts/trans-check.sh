#!/bin/sh
#
# Validate a single .po catalog for release/commit readiness.
#
# Usage: scripts/trans-check.sh [--review] <po-file>
#
# Gates, each reported by name on failure:
#   1. msgfmt -c --check-format: parses cleanly, valid header, and no
#      format-specifier type losses (%d vs %s) or count drift. msgfmt
#      reports its own file:line locations on stderr.
#   2. zero fuzzy entries; each remaining one reported with the po-file
#      line number of its msgid.
#   3. zero untranslated entries (skipped with --review); each remaining
#      one reported with the po-file line number of its msgid.
#
# Exit status: 0 all gates pass, 1 any gate fails, 2 usage error.

# Review mode gates staged partial catalogs: keep syntax, format, and fuzzy
# gates, drop the untranslated-completeness gate that a partial catalog fails.
review=false
case "${1:-}" in
	--review) review=true; shift ;;
esac

l="$1"

if [ -z "$l" ]; then
	echo "usage: $0 [--review] <po-file>" >&2
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

# Gate 2: fuzzy entries, located by the po-file line number of the msgid
# that immediately follows each "#, fuzzy" (or "#, fuzzy, ...") flag line.
fuzzy_lines=$(awk '
	/^#, *fuzzy/ { pending=NR; next }
	/^msgid "/ { if (pending) { print pending; pending=0 } }
' "$l")

if [ -n "$fuzzy_lines" ]; then
	fuzzy_count=$(printf '%s\n' "$fuzzy_lines" | wc -l)
	echo "$l: FAIL $fuzzy_count fuzzy entries remain"
	printf '%s\n' "$fuzzy_lines" | while IFS= read -r n; do
		echo "$l: FAIL fuzzy entry at line $n"
	done
	rc=1
fi

# Gate 3: untranslated entries, located by the po-file line number of each
# entry's msgid line. An entry is untranslated when its msgstr is exactly
# the empty string with no continuation lines.
if [ "$review" = false ]; then
	untranslated_lines=$(awk '
		BEGIN { msgid_line=0; in_msgstr=0; content="" }
		/^msgid "/ { msgid_line=NR; in_msgstr=0; next }
		/^msgstr "/ {
			in_msgstr=1
			line=$0
			sub(/^msgstr /, "", line)
			content=line
			next
		}
		in_msgstr && /^"/ { content = content $0; next }
		{
			if (in_msgstr && content == "\"\"") print msgid_line
			in_msgstr=0
		}
		END {
			if (in_msgstr && content == "\"\"") print msgid_line
		}
	' "$l")

	if [ -n "$untranslated_lines" ]; then
		untranslated_count=$(printf '%s\n' "$untranslated_lines" | wc -l)
		echo "$l: FAIL $untranslated_count untranslated entries remain"
		printf '%s\n' "$untranslated_lines" | while IFS= read -r n; do
			echo "$l: FAIL untranslated entry at line $n"
		done
		rc=1
	fi
fi

if [ "$rc" -eq 0 ]; then
	echo "$l: mechanical checks pass; language accuracy still requires review"
fi

exit $rc
