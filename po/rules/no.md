# Norwegian (Bokmål, `no`) translation rules

## 1. Character set, symbology, expectations

- Latin alphabet plus æ, ø, å (lowercase/uppercase pairs Æ/æ, Ø/ø, Å/å).
- Straight quotes `'...'` for inline literal names (widget ids, card mnemonics), matching existing catalog usage (eg `'%s'`); no guillemets « » in UI strings.
- Unit symbols and NEC2 mnemonics stay as in source: MHz, dBi, Ω, VSWR, GW, EX, LD, FR, RP, GE, EN, S-parameters, `.nec`/`.csv`/`.s1p`/`.s2p`/`.png`.
- Decimal comma (`50,0`) is the Norwegian norm, but numeric formatting is code-driven, not translator-driven; do not alter numbers inside format strings.
- En dash `—` in source (eg "Patch Flow — Animated") is preserved as-is; Norwegian typesetting accepts it identically to English usage here.

## 2. Technical interface writing standards

- Compound terms are written as one word per Norwegian orthography: `Strømretning`, `Jordplantype`, `Forsterkningsnormalisering`, `Feilsøkingsutdata` — never hyphenated or split like the English source.
- Sentence case, not English title case: only the first word and proper nouns are capitalized in menu items, dialog titles, and labels (eg "Jordparametere (GD-kort)", not "Jordparametere (GD-Kort)").
- Commands/buttons/menu actions use the imperative or bare infinitive/noun form with no subject pronoun (eg "Velg...", "Aktiver...", "Skriv ut..."); this is the default GNOME/GTK Norwegian localization convention and doubles as the formality register (see §3).
- GTK hotkey underscores (`_X`) are kept on a letter that is genuinely typable and does not collide with another mnemonic already used in the same menu/dialog; shift to a different letter of the Norwegian word when the literal first letter collides.
- Error/log strings routed through `pr_*` stay terse, single technical register, no punctuation flourishes beyond the source's.

## 3. Formality / informality

- Modern Bokmål software UI does not use a T-V (Du/De) pronoun distinction; "De" is archaic/formal-only and never appropriate here.
- Register is set structurally, not by pronoun choice: imperative/infinitive verb forms and impersonal noun phrases (§2) read as neutrally professional to Norwegian users — this *is* the formal register for this project, not an informal one.
- Sections 1-3 do not overlap: §1 is orthography/symbols, §2 is structural writing conventions, §3 is the (pronoun-free) address register.

## 4. NEC2 domain mapping (established lexicon — reuse, do not vary)

| English | Norwegian | Notes |
|---|---|---|
| current (electrical) | Strøm(mer) | never "nåværende" (temporal) |
| charge (electrical) | Ladning(er) | never "kostnad" (billing) |
| gain | Forsterkning | antenna directivity, never "gevinst"/"profitt" |
| magnitude | størrelse | signal/impedance magnitude; never "magnitud" (that is seismic/astronomical). Matches graph label `Z-størrelse` and tooltips (`strømmens størrelse`); homonymous with "quantity"→størrelse, which program context disambiguates |
| conductivity | ledningsevne | native term throughout (`Jordledningsevne`, `Trådledningsevne`, `Ledningsevne S/m`); never the loanword "konduktivitet" |
| ground / ground plane | Jord / Jordplan | never "bakke"/"jordsmonn" (soil); the jord stem carries every "ground" sense here — "ground effects"→`jordeffekter`, "below ground"→`under jordnivå`, never `bakke*` |
| reflect (behavioral "mirrors …") | Speiler | present-tense verb for a control/menu that tracks another (eg "Mirrors the Currents button"→`Speiler Strømmer-knappen`); distinct from the geometry imperative `Speil` (GX mirror) and the physics noun `refleksjon` |
| sky (antenna-temp) | himmel | eg `himmelmodell`, `himmel/jord-grensen`; never "himmelsone" (adds absent "zone" qualifier) |
| near-field | nærfelt | eg `nærfeltdata`, `nærfeltvektorer`; never "nærsone" |
| reflect (mirror op) | Speil (imperativ) | geometry transform verb, parallels `Flytt`/`Skaler`; noun form `Speiling` only in `Speilingsalternativer`. "refleksjon" is the physics noun (reflection coeff./angle), a distinct term |
| load (impedance) | (i sammensetning, ikke oversatt separat i denne katalogen) | electrical impedance sense only |
| patch | Patch (uoversatt) | kept as loanword per existing catalog (`Patch`, `Patch-element`, `Patch-type`, `patch-strømmer`, `patch-segmenter`, `overflate-patcher`); never "flate*"/"lapper" for patch — `flate`/`flateareal` only in the established compound `Patch Area` → `Flateareal` |
| segment, tag, wire | segment, tag, tråd | NEC2 geometry terms; "wire" → `tråd` where already translated |
| excitation | eksitasjon / eksitasjonstype | RF energy input, not emotional state |
| pattern (radiation) | (strålings)diagram / mønster | per existing "Forsterkningsdiagram" |
| VSWR, dBi, S-parametere, Ω | unchanged | locked technical symbols |
| feedpoint | matepunkt | antenna feed point; established in freqplots port-selector strings |
| Power (color/scale family name) | Effekt | family name for the power-law magnitude-to-color transfer, not "linear power" |
| Asinh, Reinhard, Sigmoid, μ-law | unchanged | named transfer-function/algorithm terms, kept as international technical loanwords like VSWR |

No qualifier is added beyond what the English source states (eg "Strømmer" alone for "Currents", not "Elektriske strømmer") — program context (NEC2 electromagnetic simulator) disambiguates for the user.
