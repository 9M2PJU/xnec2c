#!/bin/sh

# Loop the single-catalog worker over each catalog argument.
#
# Usage: scripts/trans-validate.sh [--mode new|update|validate|fast] [--model NAME] <po-file>...

usage() {
	cat >&2 <<'EOF'
usage: trans-validate.sh    [--mode new|update|validate|fast] [--model NAME] <po-file>...
       trans-validate-1.sh  [--mode new|update|validate|fast] [--model NAME] <po-file>

Loop the single-catalog worker over each catalog argument.

Leading options, shifted off before the catalogs:
  --mode new|update|validate|fast
                               force the mode for every catalog; when omitted,
                               each catalog's mode auto-detects.
  --model NAME                 override the mode's default model for every
                               catalog; when omitted, the mode default applies.
  --groups N                   partition the catalogs into N equal-sized groups
                               and print each group on one line, then exit
                               without validating.

Modes (default model):
  new       untranslated scope,   claude-sonnet-5[1m], reconcile,    full gate
  update    untranslated + fuzzy, claude-sonnet-5[1m], reconcile,    full gate
  validate  staged git hunks,     claude-opus-4-8[1m], no reconcile, --review gate
  fast      staged git hunks,     claude-opus-4-8[1m], no reconcile, --review gate, single pass

Detection precedence: no file -> new; staged edits -> validate;
zero translated entries -> new; otherwise update.

Gate: scripts/trans-check.sh [--review] <po-file>
EOF
	return 0
}

# Report the file capacity of group $1 given base size $2 and remainder $3;
# the first $3 groups absorb one extra file so sizes differ by at most one.
group_cap() {
	if [ "$1" -le "$3" ]; then
		echo $(($2 + 1))
	else
		echo "$2"
	fi
	return 0
}

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Parse the leading option flags, shifting each off before the catalogs;
# forward whichever are set to every worker invocation.
modeopt=""
modelopt=""
groupsopt=""
parsing=true
while [ "$parsing" = true ]; do
	case "$1" in
		--mode) modeopt="--mode $2"; shift 2 ;;
		--model) modelopt="--model $2"; shift 2 ;;
		--groups) groupsopt="$2"; shift 2 ;;
		*) parsing=false ;;
	esac
done

if [ -z "$1" ]; then
	usage
	exit 2
fi

# Split the catalogs into N equal-sized groups and print each group on one line
# for easy copy-paste, then exit without validating.
if [ -n "$groupsopt" ]; then
	case "$groupsopt" in
		''|*[!0-9]*) usage; exit 2 ;;
	esac
	if [ "$groupsopt" -lt 1 ]; then
		usage
		exit 2
	fi
	total=$#
	base=$((total / groupsopt))
	rem=$((total % groupsopt))
	g=1
	count=0
	cap=$(group_cap "$g" "$base" "$rem")
	line=""
	for f in "$@"; do
		if [ -z "$line" ]; then
			line="$f"
		else
			line="$line $f"
		fi
		count=$((count + 1))
		if [ "$count" -ge "$cap" ]; then
			printf '%s\n' "$line"
			g=$((g + 1))
			count=0
			line=""
			cap=$(group_cap "$g" "$base" "$rem")
		fi
	done
	if [ -n "$line" ]; then
		printf '%s\n' "$line"
	fi
	exit 0
fi

for l in "$@"; do
	echo
	echo
	echo
	echo ================================== "$l" ===============================
	if "$here/trans-validate-1.sh" $modeopt $modelopt "$l"; then
		stty echo cooked
		reset
		sleep 3
	fi
done
