# Translation frame

Restate these requirements before every response and before every file read; state once, act, do not re-derive. Two roles: translate the scoped entries of the target catalog, and record the per-language lexicon in its `po/rules/<lang>.md`.

## Access constraints

REQUIRED: Read and Edit only two files, the target catalog and its `po/rules/<lang>.md`. Evaluate this file and extend or correct it as necessary. 
REQUIRED: Run exactly the one self-check gate named in the closing reminder; run no other command. It is a sanctioned validator, not auxiliary tooling.
REQUIRED: Create no auxiliary script, test harness, or tooling.
REQUIRED: Read only the annotated line ranges with offset/limit; never load the whole catalog; read about 30 lines around each scoped entry's annotated line number; process entries in groups.
REQUIRED: Preserve every format specifier (`%s`, `%d`, `%f`, `%c`, `%%`) in each msgstr, matching its msgid set; keep source order, or reorder positionally where grammar requires with `%2$s ... %1$s`.

## Edit strategy

Minimize `old_string`/`new_string` to the least unique match; msgid is the unique key.
- Minimum match: msgid line(s) plus msgstr line(s).
- Fuzzy entry: include the flags line (eg `#, fuzzy` or `#, fuzzy, c-format`); remove the `#, fuzzy` line and any `, fuzzy` within a flags line.
- Multi-line msgid: include `msgid ""` and enough continuation lines through msgstr to be unique.
- Never match a `#:` source reference, translator comment, or `#|` previous-msgid.
- Merge adjacent entries into one Edit; give non-adjacent entries separate minimal Edits.

## Disambiguation

Terms with multiple meanings inherit their technical sense from the application domain, an electromagnetic simulator; guidance shows which meaning to choose, not that a qualifier is required.
- Drop the specific technical term when program context suffices; add it only when this language needs it to separate an unrelated domain; evaluate per language.
- eg English "View Currents" to Afrikaans "Bekyk Strome", electrical sense understood, not "Bekyk Elektriese Strome", which adds an absent qualifier.
- Translation correctness is determined by best practices in that language. It is not determined based on existing patterns because existing patterns could also be wrong. This is not about quorum, this is about correctness. If you find an invalid term, add that to the list for validation. Search to see if there's any violations, even if they're outside of the scope. 

## Casing and hotkeys

- Axis names X/Y/Z stay uppercase unless the native convention differs.
- GTK hotkeys take a preceding underscore; prevent duplicate hotkeys in one UI by shifting the underscore to another letter; choose a letter matching language expectation and genuinely typable on a common keyboard for the locale; eg Spanish `_Cancelar` and `C_errar`, not two `_C`.

## Priorities

In order:
a. Valid translation meaning.
b. Valid cultural representation for interface terminology in the using region.
c. Consistent lexicon everywhere: one correct symbol or term per meaning, in correct tense and formation, per string and across the catalog; choose the most correct term over the most common, then apply it everywhere.
d. Disambiguation only where program context is insufficient.
e. Correct locale data representation; eg decimal symbol `50.0` versus `50,0`.

## Phases

Perform each numbered step as a separate response; give each item its own thinking; summarize and prove completion before the next phase. At the outset, TaskCreate each phase and its steps in compact structured form.

### Phase 1: language evaluation

1. List the native character set, symbology standards, and expectations.
2. List the writing standards for a technical program interface in this language.
3. List formality and informality terms and their mapping to the culture using the interface; sections 1-3 must not intersect in meaning.
4. Map these to the NEC2 electromagnetic simulator domain for this language and culture.
5. Write `po/rules/<lang>.md` if it does not exist.

### Phase 2: translation

1. Read about 30 lines around the next scoped entry with offset/limit.
2. Translate the scoped entries in this region: write each untranslated msgstr; for each fuzzy entry evaluate the existing translation against the changed msgid, correct it, and remove the fuzzy marker. REQUIRED: format specifiers preserved in matching set and valid order.
3. Map every translation onto the Phase 1.1-1.4 lists.
4. Reconsider 2 and 3 against interface best practice for this language; a wrong choice can insult.
5. Apply the priority order a-e above.
6. List every violation in 2-5.
7. TaskCreate each violation with its line number or range.
8. Consider similar violations that may recur, even if not within the original scope. Global correctness is a higher priority than the initial search scope defines; if any, return to 6.
9. Return to 1 for the next scoped entry.

ultrathink

### Phase 3: apply

TaskCreate item by item, Edit each violation in the catalog.

### Phase 4: consistency sweep

A successful Edit needs no re-read; trust the tool result and reason over the loaded content and the changes applied.

1. After all scoped entries translate, sweep for recurring violations Phase 2 missed; combine searches into one or more large regular-expression unions to minimize requests.
2. List every violation found.
3. TaskCreate each with its line number or range.

### Phase 5: gate

1. Apply the Phase 4 items.
2. Reconsider exhaustively for remaining violations.
3. TaskCreate any found with line numbers.
4. Complete every open item.
5. Confirm by searching the catalog: every msgstr carries its msgid's format-specifier set in valid order; no `#, fuzzy` line or `, fuzzy` flag remains. The closing gate enforces untranslated completeness for full runs; correct any entry it names.
6. REMINDER: the closing reminder names the one self-check gate; run exactly that command and read its output; on any FAIL line correct the named entries, TaskCreate each with its line number, and re-run until it exits 0.

REMINDER: format specifiers match each msgid; no `#, fuzzy` line or `, fuzzy` flag remains; edit only the catalog and its `po/rules/<lang>.md`; finish when the closing self-check gate exits 0.
