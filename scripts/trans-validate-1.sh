#!/bin/sh

# Translate and validate a single .po catalog in one of three modes.
#
# Usage: scripts/trans-validate-1.sh [--mode new|update|validate|fast] [--model NAME] <po-file>
#
# Modes come from the table below; each row varies only four values:
# scope contributors, model, thinking budget, reconcile, gate. Adding a
# mode adds a row, never a branch. Mode resolves from the --mode option
# or from catalog and git state via detect_mode.

export CLAUDE_HOOKS_KILL_ON_STOP=done

usage() {
	echo "usage: $0 [--mode new|update|validate|fast] <po-file>" >&2
	return 0
}

# Mode table: name contributors model think reconcile gate effort catalog diff
# procedure. The catalog column injects the full catalog only when yes; update
# and validate work from the scoped subset and read the catalog on demand. The
# diff column injects the catalog's staged and working-tree git diffs when yes.
# The procedure column names the injected prompt procedure: phases.md drives the
# phased iterative review, pass.md drives a single-pass correction.
mode_table() {

#name     contribs            rowmodel             think  reconcile  gate    effort  catalog  diff  procedure
	cat <<'EOF'                                                                             
new       untranslated        claude-sonnet-5[1m]  31999  yes        full    low     yes      no    phases.md
update    untranslated,fuzzy  claude-sonnet-5[1m]  2048   yes        full    low     no       no    phases.md
validate  staged              claude-opus-4-8[1m]  31999  no         review  low     no       yes   phases.md
fast      staged              claude-opus-4-8[1m]  31999  no         review  low     no       yes   pass.md
EOF
	return 0
}

# Emit the single table row whose name matches; empty when no match.
select_row() {
	mode_table | awk -v m="$1" '$1 == m { print }'
	return 0
}

# Resolve mode from catalog and git state, single point of truth.
# Precedence: absent catalog is new; staged edits are validate; a catalog
# with no translated entries is new; otherwise update.
detect_mode() {
	if [ ! -f "$1" ]; then
		echo new
		return 0
	fi
	if git diff --cached --name-only -- "$1" | grep -q .; then
		echo validate
		return 0
	fi
	if [ "$(msgattrib --translated "$1" | grep -c '^msgid ".')" -eq 0 ]; then
		echo new
		return 0
	fi
	echo update
	return 0
}

# Scope contributors: each emits the same annotated block shape so the
# frame never varies. Annotation lines read '#   line N'; count_scope
# counts them across every contributor.

scope_untranslated() {
	printf '# Untranslated entries (line numbers in %s)\n' "$1"
	awk '/^msgstr ""$/ { n=NR; getline; if ($0 !~ /^"/) print "#   line " n }' "$1"
	printf '\n'
	msgattrib --untranslated --no-wrap "$1"
	printf '\n'
	return 0
}

scope_fuzzy() {
	printf '# Fuzzy entries (line numbers in %s)\n' "$1"
	grep -n '^#, .*fuzzy' "$1" | sed 's/^\([0-9]*\):.*/#   line \1/'
	printf '\n'
	msgattrib --only-fuzzy --no-wrap "$1"
	printf '\n'
	return 0
}

scope_staged() {
	printf '# Staged-changed entries (line numbers in %s)\n' "$1"
	git diff --cached -U0 -- "$1" | grep -oP '^@@ .*\+\K[0-9]+' \
		| while read -r n; do echo "#   line $n"; done
	printf '\n'
	return 0
}

# Union the row's comma-separated contributors into one scope file.
build_scope() {
	scope="po/.translate-scope-$2.txt"
	: > "$scope"
	old_ifs=$IFS
	IFS=,
	for c in $3; do
		"scope_$c" "$1" >> "$scope"
	done
	IFS=$old_ifs
	echo "$scope"
	return 0
}

# Count scoped entries by their uniform annotation lines.
count_scope() {
	grep -c '^#   line ' "$1"
	return 0
}

# Shared recency reminder tail; names the gate literally as the closing cue.
# Reads the resolved catalog, language, and gate flag globals at call time.
reminder_tail() {
	printf 'format specifiers match each msgid; no fuzzy entries remain; edit only %s and po/rules/%s.md; finish when scripts/trans-check.sh %s %s exits 0' \
		"$l" "$lang" "$gateflag" "$l"
	return 0
}

# Per-mode domain framing for the injected bookend; one function per mode so
# adding a mode adds a function, never a branch, matching the scope dispatch.
notice_new() {
	printf 'Translate this brand-new catalog: every entry is scoped; read each region in groups and translate all, then sweep the whole catalog for related violations and fix each one found.'
	return 0
}

notice_update() {
	printf 'Update this existing catalog: translate the scoped untranslated entries and repair the scoped fuzzy entries against their changed msgids, then sweep the whole catalog for related violations and fix any found beyond the scope.'
	return 0
}

notice_validate() {
	printf 'Review this catalog'\''s staged translations against every rule and correct them, then sweep the whole catalog for related violations and fix any found beyond the staged entries.'
	return 0
}

notice_fast() {
	printf 'Validate this catalog'\''s staged translations in a single pass from the injected diff alone: it is the complete record of every changed entry. Decide each correction against the rules and apply all edits; perform no Read, Grep, or catalog sweep, and do not split the work into phases or turns.'
	return 0
}

# Parse the leading option flags, shifting each off before the catalog argument.
mode=""
model=""
parsing=true
while [ "$parsing" = true ]; do
	case "$1" in
		--mode) mode="$2"; shift 2 ;;
		--model) model="$2"; shift 2 ;;
		*) parsing=false ;;
	esac
done

if [ -z "$1" ]; then
	usage
	exit 2
fi

# Anchor to the repository root from this script's location so every relative
# artifact path resolves regardless of the caller's working directory.
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(dirname "$here")

# Normalize the catalog to an absolute path before changing directory, then
# express it repo-root-relative for the injected records and the gate command.
l="$1"
case "$l" in
	/*) ;;
	*) l="$PWD/$l" ;;
esac
cd "$root" || exit 2
l=${l#"$root"/}

lang=$(basename "$l" .po)
mode=${mode:-$(detect_mode "$l")}

read name contribs rowmodel think reconcile gate effort catalog diff procedure <<EOF
$(select_row "$mode")
EOF

if [ -z "$name" ]; then
	echo "$l: unknown mode '$mode'" >&2
	exit 2
fi

# The --model override wins over the row's default model.
model=${model:-$rowmodel}

export MAX_THINKING_TOKENS="$think"

# Raise reasoning effort only for the row that requests it; the '-' sentinel
# leaves the environment default in place for the other modes.
[ "$effort" != - ] && export CLAUDE_CODE_EFFORT_LEVEL="$effort"

scope=$(build_scope "$l" "$lang" "$contribs")
count=$(count_scope "$scope")

if [ "$count" -eq 0 ]; then
	echo "$l: nothing to translate in $mode mode"
	rm -f "$scope"
	exit 1
fi

echo "$l: $mode mode, $count scoped entries"

# Resolve the gate flag from the row; review gates format and fuzzy only.
gateflag=""
[ "$gate" = review ] && gateflag="--review"

# Domain-specific opening notice plus the shared recency reminder: the notice
# frames the mode, the tail names the gate as the closing cue. Both anchor the
# claude-inject bookends, dispatched by mode name from the mode table.
reminder="$("notice_$mode"). REMINDER: $(reminder_tail)"

# Injection cannot open a missing file; seed an absent per-language rules file
# with a fill-me stub so the frame's Phase 1 populates it in place instead of
# aborting injection and skipping the catalog.
if [ ! -f "po/rules/$lang.md" ]; then
	printf '# %s translation rules\n\nNeeds to be filled out.\n' "$lang" > "po/rules/$lang.md"
fi

# Add the full catalog to the injection list only when the row requests it;
# update and validate omit it and read the scoped regions on demand.
catalogarg=""
[ "$catalog" = yes ] && catalogarg="$l"

# Build the diff manifest only when the row requests it; each line runs a
# catalog-scoped git diff so claude-inject injects the staged and working-tree
# changes as synthetic command results. A manifest keeps each multi-word
# command as one token, immune to word-splitting on expansion.
diffarg=""
if [ "$diff" = yes ]; then
	manifest="po/.translate-diff-$lang.txt"
	printf '!git diff --staged -- %s | grep -P -C1 "^[+](msgid|msgstr)"\n!git diff -- %s | grep -P -C1 "^[+](msgid|msgstr)"\n' "$l" "$l" > "$manifest"
	diffarg="@$manifest"
fi

# Hand off through claude-inject so frame, reference, catalog, and scope load
# untruncated as synthetic records, separated from the reminder bookends.
claude-inject --claude claude-xraw --cwd "$root" --permission-mode acceptEdits --model "$model" \
	po/prompt/frame.md "po/prompt/$procedure" doc/TRANSLATING.md "po/rules/$lang.md" "$scope" $catalogarg $diffarg \
	-- "$reminder"

# Reconcile against the template only for full-catalog modes; validate leaves
# the staged catalog unmerged.
if [ "$reconcile" = yes ]; then
	msgmerge --update --backup=none "$l" po/xnec2c.pot
	msgattrib --no-obsolete -o "$l" "$l"
fi

rm -f "$scope"
[ -n "$diffarg" ] && rm -f "${diffarg#@}"

# Confirm the catalog passes the row's gate set.
scripts/trans-check.sh $gateflag "$l"
exit $?
