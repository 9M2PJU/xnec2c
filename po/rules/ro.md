# ro (Romanian) translation rules

## 1. Character set / symbology

- Modern Romanian orthography (DOOM2, post-1993): use comma-below diacritics
  `ă â î ș ț` (U+0103, U+00E2, U+00EE, U+0219, U+021B). Never use the Turkish
  cedilla look-alikes `ş ţ` (U+015F, U+0163).
- `â` inside a word, `î` word-initial/word-final (eg `în`, `hotărî`, `pârâu`).
- Latin alphabet, standard uppercase/lowercase; no separate technical
  symbology beyond standard SI/electrical unit symbols, which stay as in the
  source (`Ω`, `dBi`, `MHz`, `%`, `°`).
- Decimal separator in running Romanian prose is the comma, but numeric
  literals embedded in msgids/format output are not localized here (lowest
  priority, item e); leave `%d`/`%f`-produced numbers exactly as formatted by
  the program.

## 2. Technical interface writing standards

- Sentence case for labels and menu items: capitalize only the first word and
  proper nouns/acronyms (NEC2, VSWR, GW, EX, LD, FR, RP...). Do not
  title-case every word.
- UI commands (buttons, menu actions): short imperative/infinitive-derived
  noun or verb phrase, no explicit pronoun (Romanian imperative singular
  carries no informality marker in software convention, eg "Salvează",
  "Deschide", "Resetare").
- Status/log/error messages (`pr_*`, `g_message`, etc.): impersonal
  reflexive or passive-like constructions matching existing translation
  memory in this catalog, eg "se ignoră...", "nu s-a putut crea...",
  "...a eșuat". Do not address the user directly in these strings.
- Confirmation dialogs addressed to the user: formal/impersonal register
  (eg "Sigur doriți să...", "Sunteți sigur..."), never the informal "tu" form.
- Keep NEC2 mnemonics, file extensions, and unit symbols unchanged per
  doc/TRANSLATING.md.
- The generic noun `card`/`carduri` is lowercase mid-phrase, including inside
  parentheticals (eg `Comandă de excitație (card EX)`, `Parametri de sol
  (carduri GN și GD)`); it is capitalized only when it is the first word of a
  standalone label (eg `Card GH`, `Card SC`).
- Coordinated option names that pair two distinct physical quantities or modes
  with `/` or `+` capitalize each element, mirroring the source label
  (eg `Staționară/Călătoare`, `Curent + Sarcină`); this is the one exception to
  sentence case, because each element names an independent quantity.

## 3. Formality mapping

- Romanian software localization (GNOME/KDE ro convention) does not use an
  explicit "dumneavoastră" pronoun in short commands; formality is carried by
  register/word choice, not pronoun insertion.
- User-facing dialogs and warnings: formal, professional, impersonal tone.
- Never use informal second-person singular verb forms ("tu" register, eg
  "salvezi", "vrei") anywhere in this interface.
- Sections 1-3 above are independent: character set is orthography, writing
  standards are register/structure, formality is pronoun/verb-form choice.

## 4. NEC2 domain mapping (Romanian electrical engineering)

| English | Romanian | Notes |
|---|---|---|
| current (electrical) | curent | eg "Curenți" (Currents) |
| charge (electrical) | sarcină / sarcini | electric charge, Coulombs |
| load (impedance) | sarcină | same word as "charge" in Romanian EE usage; correct per context, not an error |
| gain | câștig | antenna directivity, dB |
| impedance | impedanță | |
| excitation | excitație | |
| wire (NEC2 element) | fir / fire | antenna conductor segment |
| wireframe (compound) | cadru de sârmă | established TM idiom, "sârmă" only in this compound |
| pattern (radiation) | diagramă (de radiație) | |
| ground / ground plane | masă / plan de masă | electrical reference, not soil |
| patch (NEC2 surface element) | Patch | kept as established loanword per existing TM |
| tag (NEC2 identifier) | tag | loanword, not "etichetă"; lowercase mid-phrase (eg "Număr tag", "Increment tag") |
| segment (NEC2 geometry) | segment | native Romanian cognate, translate |
| polarity (sign of a value) | polaritate | eg hue-sign toggle; NOT "polarizare" |
| polarization (antenna/wave) | polarizare | EM field orientation; distinct from "polaritate" above |
| Total field | Câmp total | |
| flow direction | direcție flux | |

## 5. Disambiguation

- Do not add qualifiers absent from the source (eg "Curenți" not "Curenți
  electrici") when program/domain context already disambiguates.
- "sarcină" legitimately serves both "load" and "charge" in Romanian
  electrical-engineering register, mirroring the English overload; resolve
  per surrounding context, no extra qualifier needed.

## 6. Format specifiers

- Preserve every `%s %d %f %c %%` from the msgid, same order, in msgstr;
  Romanian word order rarely requires positional reordering, but use
  `%2$s`/`%1$s` if grammar demands it.
