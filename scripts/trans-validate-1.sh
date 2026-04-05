#!/bin/sh

# Incremental translation update for a single .po file.
#
# Usage: scripts/trans-validate-1.sh <po-file>
#
# Translates only untranslated and fuzzy entries.

l="$1"

#MODEL=opus
MODEL=sonnet
export MAX_THINKING_TOKENS=2048

# Build scope: untranslated + fuzzy entries with line numbers
scope="po/.translate-scope-$(basename "$l" .po).txt"

printf '# Untranslated entries (line numbers in %s)\n' "$l" > "$scope"
awk '/^msgstr ""$/ { n=NR; getline; if ($0 !~ /^"/) print "#   line " n }' "$l" >> "$scope"
printf '\n' >> "$scope"
msgattrib --untranslated --no-wrap "$l" >> "$scope"
printf '\n# Fuzzy entries (line numbers in %s)\n' "$l" >> "$scope"
grep -n '^#, .*fuzzy' "$l" | sed 's/^\([0-9]*\):.*/# line \1/' >> "$scope"
printf '\n' >> "$scope"
msgattrib --only-fuzzy --no-wrap "$l" >> "$scope"

count_u=$(msgattrib --untranslated "$l" | grep -c '^msgid ".')
count_f=$(msgattrib --only-fuzzy "$l" | grep -c '^msgid ".')

if [ "$count_u" -eq 0 ] && [ "$count_f" -eq 0 ]; then
	echo "$l: fully translated"
	rm -f "$scope"
	exit 0
fi

echo "$l: $count_u untranslated, $count_f fuzzy"

claude-raw --permission-mode acceptEdits --model "$MODEL" "@doc/TRANSLATING.md @$scope ; $l has $(wc -l < "$l") lines, $count_u untranslated and $count_f fuzzy entries to translate:

The scope file loaded above contains the specific entries requiring translation, annotated with their line numbers in $l.
For each scoped entry, Read approximately 30 lines around its line number in $l using offset/limit. Do not read the entire file. Process entries in groups to maintain focus.

NOTICE:
- $l is the ONLY file you may Read/Edit.
- You MUST ONLY Read/Edit $l because all other tools are forbidden.
- Evaluation tasks operate on $l only: Do not create auxiliary scripts, test harnesses, or tooling

Before each response, before each file read, restate the requirements. Read only the relevant line ranges using offset/limit; do not load the entire file.

### Edit Strategy for PO Files

Minimize context in old_string/new_string to reduce token usage:
- Minimum unique match: msgid line(s) + msgstr line(s). msgid is the unique key in PO files.
- For fuzzy entries: include the flags line (#, fuzzy or #, fuzzy, c-format, etc.) since it must be removed.
- For multi-line msgid: include msgid \"\" and enough continuation lines to be unique, through msgstr.
- Never include #: source references, translator comments, or #| previous-msgid in the match.
- Merge sequential adjacent entries into a single Edit call.
- Non-adjacent entries: separate Edit calls, each with minimal matching.

### Context-Dependent Disambiguation

Terms with multiple meanings inherit their technical sense from application domain.
Disambiguation guidance shows which meaning to choose, not that qualifiers are required.

EXAMPLES (adapt to each situation):
	English: View Currents → Afrikaans: Bekyk Strome (electrical sense understood)
	NOT: Bekyk Elektriese Strome (adds qualifier absent in source)

The application (electromagnetic simulation) disambiguates for users UNLESS
this particular language needs the extra specific term to disambiguate from
an unrelated domain. If the program context is sufficient, then we can drop
the specific technical term, but otherwise include it. This is per
language, so you need to evaluate accordingly.

This prevents over-correction while maintaining technical accuracy.

Axis names like X/Y/Z must be uppercase unless the native language convention is different

GTK hotkeys use preceding underscores. Prevent duplicates in the same UI by shifting the underscore to another letter. Example: Avoid Spanish Cancel/Close colliding as _Cancelar and _Cerrar by using (eg) _Cancelar and C_errar instead.Choose the most appropriate hotkey letter based on language expectations. 

# Procedure

The steps of each phase are enumerated below. Do not combine multiple steps into the same response: You must perform each step as a separate response to provide sufficient thinking for each item.

## Phase 1: evaluation

Your responses to each evaluation section must not duplicate any meaning from an earlier section. Meaning within 1.1-1.3 must contain no intersection.

1. List native language character set/symbology standards and expectations
2. List all standards of writing with respect to the language being translated for the purpose of a technical computer program interface ($l)
3. List all informality/formality terms and how they map to the cultural expectation of the society that will be using a technical program interface
4. Map these as appropriate to the specific NEC2 EM simulator domain for this language and culture

## Phase 2: inspection

1. Read approximately 30 lines around the next scoped entry's line number using offset/limit
2. Translate scoped entries within this region: for each untranslated entry (empty msgstr), write the translation. For each fuzzy entry, evaluate the existing translation against the changed msgid, correct it, and remove the fuzzy marker (#, fuzzy line AND the comma-fuzzy within any flags line). Verify format specifiers (%s, %d, %f, %c, %%) are preserved in identical order.
3. Verify that all translations map onto the enumerated lists you provided in Phase 1: 1.1-1.4
4. Validate #2 and #3 again: does it follow all best practices considering computer user interface requirements for this particular language ($l)? This is critically important to avoid potential insult.
5. List priorities, in order:
	a. Valid translation meaning
	b. Valid cultural representation with respect to interface terminology expectations for the region that uses the language.
	c. Consistent lexicon everywhere:
		- Use the most appropriate symbol or term for the same purpose/meaning, and in the proper formation (eg, tense) in relation to (i) each translation string and (ii) the document as a whole.
		- Choose the term that is the most correct for the situation, not necessarily the term that is most commonly used in the current translations
		- Choose the correct term and then replace it everywhere. Correctness is a higher priority even if it means more modification.
	d. Appropriate disambiguation only as necessary if the program context (EM Simulator) is not sufficient.
	e. Correct locale representations of data: decimal symbol (50.0 vs 50,0), etc.
6. List all violations in #2-5
7. Add all violations from #6 using TodoWrite - include line numbers or ranges as a reminder
8. Based on your findings, consider additional validation issues that are similar to the ones you discovered, because the similar errors may have happened multiple times. If any, goto #6
9. Goto 1, process next scoped entry

## Phase 3: Update

1. For each Todo item: MUST modify any violations using the Edit tool

## Phase 4: QA/Validation

1. After all scoped entries have been translated, perform final analysis to make sure Phase 2-4 are complete.
	- If you applied a modification and the modification was successful, then you do not need to check if it changed. The tool succeeded. If the tool succeeded, then you do not need to check if it changed.
	- The purpose of this is for you to think really hard about the file content that was loaded and the modifications you made and evaluate if there's anything else that should be considered to follow the required rule set and the phase procedures.
	- Verify consistency: Perform searches to see if there are any common, similar, or repeating violations that were not detected based on the known violations from Phase 2. You MUST combine searches into one or more large regular expression unions to minimize the number of requests and reduce latency.
2. List all violations in #1
3. Add all violations from #2 using TodoWrite - include line numbers or ranges as a reminder

## Phase 5: Fine tuning

1. Apply the to-do items discovered in phase four.
2. Think hard, exhaustively, to make sure all phases are complete.
3. List all violations in #2
4. Add all violations from #3 using TodoWrite - include line numbers or ranges as a reminder
5. Complete all items on the to-do list.
6. When complete with ALL changes, you MUST issue Bash(kill -9 \$PPID). You MUST NOT run any other commands. The Todo item 'all requirements are complete' must be set to pending before invoking Bash.

# Begin Phases

- Provide a summary after each phase completion before proceeding to the next, and provide proof that the phase is complete before the next one is started.
- TodoWrite - add each Phase major and minor section to the list in a structured form to minimize list size while capturing all meaning, now
- TodoWrite - add Bash(kill -9 \$PPID) to the list, now

ultrathink
	"

# Reformat to match canonical msgmerge output and remove obsolete entries
msgmerge --update --backup=none "$l" po/xnec2c.pot
msgattrib --no-obsolete -o "$l" "$l"

rm -f "$scope"
