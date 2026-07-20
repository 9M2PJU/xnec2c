# Single-pass procedure

The injected staged and working-tree diffs are the complete, authoritative record of every changed entry. Do everything below in one pass, working only from what is already in context. Perform no Read, no Grep, no catalog sweep, and no line-range inspection; the diff already carries every entry you may correct.

## Steps

1. Take the changed entries from the injected diff; each `+msgid`/`+msgstr` pair is a candidate for correction.
2. Apply the existing `po/rules/<lang>.md` lexicon; correct that file in place only when a rule it states is missing or wrong.
3. Decide each correction against the access constraints, edit strategy, disambiguation, casing, and priority order stated in the frame, using only the diff content.
4. Preserve every format specifier in its matching set and valid order.
5. Apply every correction as a minimal Edit, building each `old_string` from the msgstr text shown in the diff and batching adjacent entries.
6. Run exactly the one self-check gate named in the closing reminder; correct any FAIL line from the diff already in context, then finish.

ultrathink
