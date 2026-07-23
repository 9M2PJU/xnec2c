# da translation rules

## 1. Character set, symbology, expectations

- Danish Latin alphabet adds æ, ø, å (Æ, Ø, Å) after z; no other diacritics in native vocabulary.
- Decimal separator is comma: `50,0` not `50.0`. Thousands grouping uses period or thin space, never comma. Do not alter numerals embedded in format specifiers or literal example values inside msgids that are code output.
- Quotation marks: UI strings use plain straight quotes `"..."` to match existing catalog convention (typographic »...« reserved for prose, not used here).
- No space before `%`, `:`, `;`, `?`, `!` (unlike French); Danish follows English spacing conventions here, e.g. `Gamma:` -> `Gamma:`.
- Compound technical nouns are written as one word per Danish orthography: `strømfordeling` (current distribution), `hukommelsesallokering` (memory allocation), `strømningsretning` (flow direction) — never hyphenated or split.
- Units and mnemonics stay unchanged per doc/TRANSLATING.md: MHz, dBi, Ω, degrees, VSWR, NEC2 card mnemonics (GW, EX, LD, FR, RP, GE, EN), file extensions (.nec, .csv, .s1p, .s2p, .png).

## 2. Technical interface writing standards

- Sentence case for labels, buttons, and menu items (not English Title Case): `Vælg fil` not `Vælg Fil`.
- Prefer imperative mood for actions/buttons: `Gem`, `Nulstil`, `Vælg`.
- Definite article is a suffix in Danish (`-en`/`-et`/`-ne`), used only when grammatically required; do not add an English-style leading "the".
- Established loanwords are kept when they are the normal Danish technical term (`software`, `backtrace`, `widget`-derived internal terms in developer/pr_* strings), but everyday English is translated.
- Developer/debug strings (pr_debug, internal `%s: ...` diagnostic messages) are translated for consistency with the rest of the catalog since this catalog already fully translates them; keep terse, technical register matching the English original.

## 3. Formality mapping

- Modern Danish software interfaces use the informal `du` register or, more commonly, avoid direct address altogether via imperative verb forms and impersonal phrasing (`Vælg`, `Angiv`, `Kunne ikke...`). The formal `De` is reserved for legal/ceremonial text and is not used in this interface.
- This differs from German/French/Spanish/Italian/Turkish/Russian, which mandate a formal register (doc/TRANSLATING.md); Danish technical UI convention (per Danish IT/localization practice, e.g. Microsoft's Danish style guide) defaults to imperative/impersonal phrasing, with `du` only if second person is unavoidable.
- Error and status messages use impersonal/declarative construction: `Kan ikke gemme...`, `mislykkedes`, not `De kan ikke...`.

## 4. NEC2 domain mapping (established lexicon in this catalog)

| English | Danish | Notes |
|---|---|---|
| current (electrical) | strøm / Strømme | electrical sense only, no qualifier added |
| charge (electrical) | ladning / Ladninger | |
| ground / ground plane | jord / Jordplan | |
| impedance | impedans | |
| excitation | excitation | kept as loanword, matches existing catalog |
| geometry | geometri | |
| pattern (radiation) | strålingsdiagram | established dominant catalog term; do not use "strålingsmønster" (radiation pattern = diagram, not mønster) |
| magnitude | størrelse | one term everywhere; reserve "amplitude" for English "amplitude" only (eg color projection) |
| viewer (gain/direction) | Visning / Visnings- | one term everywhere; not "Fremviser" or "betragter" |
| gain | forstærkning | antenna directivity, not profit |
| wire | tråd | |
| segment / patch / tag | segment / patch / tag | NEC2 terms, unchanged per doc/TRANSLATING.md |
| phase | fase | |
| scale | skala / skalér | |
| flow direction | strømningsretning | one word, established at line 375 |
| wireframe | trådgitter | |
| polarity (color sign, diverging hue) | polaritet | electrical/signal sign sense; keep distinct from antenna "polarization" = polarisering (established, unrelated concept, both live in this catalog) |
| far-field contribution (chroma projection) | fjernfeltbidrag | established at animate.glade:166; not "nærfelt-animation" (that string is reserved for the separate Near Field Animation dialog title) |
| instantaneous (chroma projection label) | øjeblikkelig | bare label, no "(φ=0)" suffix once msgid dropped it; the qualifier lives in the longer tooltip strings instead |
| comet (color overlay) | komet | animated bright-head overlay; not "geometri" |
| hue encoding / brightness encoding (internal) | farvetonekodning / lysstyrkekodning | distinct internal enums; do not collapse both into "farveprojektion" |
| palette kind | palettetype | |
| color tone (scale/transfer family enum) | farvefamilie | matches "Skalafamilie:" label family (Power/Log/μ-law/Reinhard/Sigmoid/Asinh/Identity) |

## 5. Priority application

Follow doc priority order a-e: correct technical meaning first, then Danish interface convention (sentence case, imperative), then lexicon consistency across the whole catalog (reuse the table above), then disambiguation only if program context is insufficient, then locale numeral formatting.
