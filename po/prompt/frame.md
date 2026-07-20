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

REMINDER: format specifiers match each msgid; no `#, fuzzy` line or `, fuzzy` flag remains; edit only the catalog and its `po/rules/<lang>.md`; finish when the closing self-check gate exits 0.
