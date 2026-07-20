# Phased procedure

Perform each numbered step as a separate response; give each item its own thinking; summarize and prove completion before the next phase. At the outset, TaskCreate each phase and its steps in compact structured form.

## Phase 1: language evaluation

1. List the native character set, symbology standards, and expectations.
2. List the writing standards for a technical program interface in this language.
3. List formality and informality terms and their mapping to the culture using the interface; sections 1-3 must not intersect in meaning.
4. Map these to the NEC2 electromagnetic simulator domain for this language and culture.
5. Write `po/rules/<lang>.md` if it does not exist.

## Phase 2: translation

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

## Phase 3: apply

TaskCreate item by item, Edit each violation in the catalog.

## Phase 4: consistency sweep

A successful Edit needs no re-read; trust the tool result and reason over the loaded content and the changes applied.

1. After all scoped entries translate, sweep for recurring violations Phase 2 missed; combine searches into one or more large regular-expression unions to minimize requests.
2. List every violation found.
3. TaskCreate each with its line number or range.

## Phase 5: gate

1. Apply the Phase 4 items.
2. Reconsider exhaustively for remaining violations.
3. TaskCreate any found with line numbers.
4. Complete every open item.
5. Confirm by searching the catalog: every msgstr carries its msgid's format-specifier set in valid order; no `#, fuzzy` line or `, fuzzy` flag remains. The closing gate enforces untranslated completeness for full runs; correct any entry it names.
6. REMINDER: the closing reminder names the one self-check gate; run exactly that command and read its output; on any FAIL line correct the named entries, TaskCreate each with its line number, and re-run until it exits 0.
