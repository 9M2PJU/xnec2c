# sl translation rules

## Character set and symbology
- Latin script with diacritics č, š, ž (Č, Š, Ž); native Slovenian keyboard has dedicated keys for these.
- Decimal separator: comma (`50,0`). Thousands separator: dot or non-breaking space.
- Quotation marks: „…" (low-high double) when quoting; straight quotes retained inside preformatted/technical strings from source.
- No case-folding ambiguity beyond standard Latin capitalization.

## Interface writing standards
- Sentence case for titles, menu items, dialogs (not Title Case per word); capitalize first word and proper nouns only.
- Commands/buttons/menu actions: imperative mood, no explicit pronoun (eg "Shrani" = Save, "Prekliči" = Cancel, "Ponastavi" = Reset).
- Established GNOME/KDE Slovenian glossary terms reused: Datoteka=File, Uredi=Edit, Pogled=View, Pomoč=Help, Nastavitve=Settings, V redu=OK, Predogled=Preview, Izhod=Exit, Zapri=Close, Odpri=Open.

## Formality
- Avoid direct pronouns in commands/labels; imperative alone is standard register for professional Slovenian software.
- Where direct address to the user is unavoidable (eg confirmation dialogs "Are you sure..."), use formal plural "ste"/"vi" register, never informal "ti"/"si".

## NEC2 domain lexicon (Slovenian)
| English | Slovenian | Notes |
|---|---|---|
| current | tok | electrical (Amperes), not "trenutni" |
| charge | naboj | electrical charge |
| ground plane | ozemljitvena ravnina / masa | RF reference, not soil |
| gain | ojačitev | antenna directivity (dB), not profit; catalog-consistent term (not "ojačenje"; avoid "dobitek" = winnings) |
| radiation pattern | vzorec sevanja | antenna directional response |
| excitation | vzbujanje | EM energy input, not emotional state |
| load | obremenitev / impedanca | electrical impedance, not weight |
| wire | žica | thin conductor |
| patch | patch | NEC2 surface element, kept untranslated (per TRANSLATING.md never-translate list, like segment/tag); decline as masculine loanword patch/patcha/patchi/patchev; never "ploskev" |
| viewer | pregledovalnik | structure/pattern viewer direction (eg "Viewer Gain" = "Ojačitev pregledovalnika"); not "opazovalec" |
| segment, tag, GW/GA/GH/EX/LD/FR/RP/GE/EN | (untranslated) | NEC2 identifiers/mnemonics |
| phase | faza | |
| flow direction | smer toka | existing catalog usage |
| wireframe | žičnati prikaz | existing catalog usage |
| peak magnitude | vrhovna amplituda | existing catalog usage |
| reference phase | referenčna faza | existing catalog usage |

## Disambiguation
- Use domain-correct technical sense without adding qualifiers absent from English source (eg "Tokovi" not "Električni tokovi" for "Currents").

## Format specifiers
- Preserve `%s %d %f %c %%` in source order in every msgstr; Slovenian free word order allows natural sentence flow around fixed specifier positions.
