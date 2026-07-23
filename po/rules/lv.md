# lv translation rules

## 1. Character set, symbology, expectations

- Latin script with Latvian diacritics: ā, č, ē, ģ, ī, ķ, ļ, ņ, š, ū, ž. Always use the precomposed diacritic forms, never combining-mark sequences.
- Decimal separator is the comma (`50,0`), not the period, in any newly authored example text; do not alter numeric literals already fixed by format specifiers (`%d`, `%f`, etc.) or by unit strings such as `MHz`, `dBi`, `Ω` — those stay as in the source.
- No case distinction rules beyond standard Latin capitalization: sentence case for labels, capitalize the first word of a title/menu entry only (no German-style noun capitalization).
- Retain NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN...), file extensions (.nec, .csv, .s1p, .s2p, .png), and unit symbols (MHz, dBi, Ω, VSWR, dB) unmodified.
- GTK hotkey underscores: keep one `_` per label; when the natural Latvian first letter collides with another mnemonic already used on the same menu/dialog, shift to a distinctive interior consonant that is easy to reach on a standard Latvian/US keyboard (Latvian layout is Latin-based, no special hotkey letters needed).

## 2. Technical program interface writing standard

- Menu items, buttons, and command labels use the infinitive/noun form rather than an imperative verb form: "Saglabāt" (to save), "Aizvērt" (to close), "Mērogot" (to scale) — this is the established convention already used throughout this catalog and matches general Latvian software-localization practice.
- Dialog questions and confirmations addressed to the user use the formal plural verb form ("vai vēlaties", "vai esat pārliecināts") without necessarily spelling out "Jūs"; Latvian freely drops the subject pronoun when the verb ending already marks formal address.
- Tooltips and status/help strings use short declarative sentences, third-person or impersonal constructions ("Atlasīt, kā ...", "Parāda ...", "Norāda ...").
- Error/log messages (`pr_*` sourced strings) are terse, impersonal, and technical; keep them literal and unembellished, matching the low-formality register already present in the catalog's existing `src/*.c` entries.
- Compound technical nouns follow Latvian genitive-chain compounding ("krāsu projekcijas saime", "signāla amplitūdas") rather than English-style noun-stacking; reorder as Latvian grammar requires while keeping format specifiers in source order.

## 3. Formality / informality mapping

- Formal register only: the professional engineering audience (RF/antenna engineers, amateur radio operators) expects the neutral-formal register already established in the file; never use the informal "tu" address.
- No separate informal tier exists for this catalog — sections 1 and 2 already fix the register; this section only confirms no additional formal/informal split is needed beyond avoiding "tu".

## 4. NEC2 domain mapping (established lexicon — reuse verbatim, do not introduce synonyms)

| English | Latvian | Notes |
|---|---|---|
| current (electrical) | strāva / strāvas | never "pašreizējais" (temporal) |
| charge (electrical) | lādiņš / lādiņi | never "maksa" (billing) |
| ground / ground plane | zeme / zemes plakne | never "augsne"; never "zemējums" (grounding/earthing) for the NEC2 earth/ground concept — use "zeme" everywhere, incl. "ground present", "with ground specified", "Sommerfeld ground" |
| wire | vads | never "kabelis/vadi=cables" |
| gain | pastiprinājums | antenna directivity, dB |
| pattern (radiation) | (starojuma) diagramma | never "veidne/paraugs" |
| excitation | ierosme | never emotional sense |
| load (impedance) | slodze | never "svars" |
| impedance | impedance | loanword; the real part of Z is resistance, so keep impedance distinct from resistance ("pretestība") and from "slodze" (load). Consistent across "Impedance (real/imag)", "Impedance vs Frequency", "Reference impedance Z0", "Transmission Line impedance" |
| resistance | pretestība | keep distinct from impedance and from "slodze" (load) |
| scale / scaling | mērogs / mērogot | consistent across GS card and color-scale UI |
| magnitude / amplitude | amplitūda | keep distinct from "lielums" (generic value) |
| flow (patch/current flow) | plūsma | e.g. "Plūsmas virziens" |
| phase | fāze | |
| widget | logrīks | established in existing `config_*` messages |
| color projection | krāsu projekcija | |
| polarity | polaritāte | |
| polarization | polarizācija | |
| feedpoint | barošanas punkts | |
| port (EX excitation port) | ports | direct loan, standard in RF Latvian usage |
| patch (NEC2 surface patch) | ielāps | established, e.g. "virsmas ielāpi" |
| segment | segments | direct loan |
| tag | tags | direct loan, standard NEC2/RF Latvian usage |
| theme (UI) | tēma | |
| brightness | spilgtums | |
| softening / knee | mīkstināšana / lūzuma punkts | |
| contrast | kontrasts | |
| compression | kompresija | |
| dynamic range | dinamiskais diapazons | |
| validation | validācija | |
| managed allocator / memory report | pārvaldītais atmiņas piešķīrējs / atmiņas pārskats | |
| polarity (sign, +/-) | polaritāte | never "polarizācija" (a distinct concept, antenna polarization) |
| polarization (antenna) | polarizācija | keep distinct from "polaritāte" (polarity/sign) |
| far field / far-field contribution | tālais lauks / tālā lauka ieguldījums | paired with existing "tuvais lauks" (near field) |
| standing wave / traveling wave | stāvvilnis / skrejvilnis | short noun labels for the Standing/Traveling color projection |
| comet (overlay trailing the wave crest) | komēta / komētas galva | use "galva" (head), not diminutive "galviņa" |
| hue encoding / brightness encoding | toņa kodējums / spilgtuma kodējums | distinct from "krāsu projekcija" (color projection, the user-facing mode name) |
| family (of curves/scales/projections) | saime | mathematical/technical family, e.g. "mēroga saime" (scale family), "krāsu projekcijas saime"; never "ģimene" (a human/social family) |
| color tone (scale-family enum, src/color/color_tone.c) | krāsu tonis | distinct from "mēroga saime" (Scale family: UI label) though same underlying concept |
| palette kind | paletes veids | fallback/default palette referred to as "gradients" (ramp) |
| node / antinode (standing-wave) | mezgls / pretmezgls | distinct wording from "nulle"/"maksimums" (null/peak) used in the tick-mark tooltip for the same feature |

- Keep VSWR, S-parameters, Z (impedance symbol), dBi, MHz, Ω, and NEC2 card mnemonics untranslated per `doc/TRANSLATING.md`.
- Do not add qualifiers absent from the English source (e.g. "elektriskā strāva" when the source just says "Currents"); the antenna-simulator context already disambiguates.
