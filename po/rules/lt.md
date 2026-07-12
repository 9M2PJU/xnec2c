# lt translation rules

## 1. Character set, symbology, expectations

- Latin script with Lithuanian diacritics: ą, č, ę, ė, į, š, ų, ū, ž (and uppercase forms). Use them correctly; do not substitute plain a/c/e/i/s/u/z.
- No case-folding rules like German noun capitalization; Lithuanian uses sentence case for labels (capitalize only the first word and proper nouns), matching English source casing style.
- No RTL, no special script shaping.
- Unit symbols stay as in English source: MHz, dBi, Ω, VSWR, degrees. NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN) stay untranslated. File extensions (.nec, .csv, .s1p, .s2p, .png) stay untranslated.
- Decimal separator in Lithuanian prose is a comma, but format strings such as `10^(-R/20)` are mathematical notation, not localized numerals, and stay unchanged; do not alter numeric literals or formula fragments embedded in a msgid.

## 2. Technical writing standards

- Lithuanian software UI favors infinitive verb forms for commands/buttons over imperative or explicit second-person conjugation (eg "Įrašyti" for "Save", "Atšaukti" for "Cancel"). This sidesteps formal/informal choice for command labels.
- Menu items and labels: noun phrases, sentence case, colon after a field label as in English ("Gama:", "Kontrastas:").
- Compound technical terms are usually native Lithuanian nouns with engineering suffixes (-imas, -umas) rather than raw loanwords: "stiprinimas" (gain), "sužadinimas" (excitation), "įžeminimas" (ground), "varža" (impedance), "slopinimas" (compression/attenuation).
- Established lexicon already present in this catalog (keep consistent, priority c):
  - Currents → Srovės
  - Charges → Krūviai
  - Wire → Laidas (dominant, correct sense across ~30 sites: Wire X1/Y1/Z1, Wire Length, Wire Data, Wire Conductivity, Radial wire, thin/tapered wire kernel messages; "viela" was a minority outlier and has been unified to "laidas")
  - Wireframe (3D render mode) → vielinis karkasas (a separate, idiomatic compound for the wire-mesh visualization; keep this one exception, do not convert to "laidinis karkasas")
  - Patch → Lopinėlis
  - Excitation → Sužadinimas
  - Frequency → Dažnis
  - Gain → Stiprinimas
  - VSWR → VSWR (kept as-is, standard abbreviation)
  - Polarity → poliarumas (the instantaneous sign/direction; NEVER "poliarizacija", which is reserved for Polarization to avoid a lexicon collision, eg "Instantaneous Polarity" → "Momentinis poliarumas")
  - Polarization → poliarizacija ("Polarization axis" → "Poliarizacijos ašis")
  - Magnitude → dydis (eg "Instantaneous Magnitude" → "Momentinis dydis", "Peak Magnitude" → "Didžiausias dydis"); keep distinct from Value → reikšmė ("Peak Value" → "Didžiausia reikšmė") so the two separate UI options never collapse to the same label
  - real part → realioji dalis (real./realioji), imag part → menamoji dalis (menam./menamoji); NEVER "tikroji/įsivaizduojamoji" for complex-number parts (eg "Impedance (real/imag)" → "Varža (real./menam.)")
  - Viewer (observer/3D-view direction) → peržiūra / peržiūros (eg "Viewer Gain" → "Peržiūros stiprinimas", "Gain in Viewer Direction" → "Stiprinimas peržiūros kryptimi"); keep one term, do not mix with "stebėtojas"
- Parenthetical mode qualifiers "(static)"/"(animated)" agree in gender with the head noun of the label: masculine karkasas → "(animuotas)"/"(statinis)"; feminine ašis/fazė/tekstūra → "(statinė)"/"(animuota)".

## 3. Formality mapping

- Direct address to the user (dialog questions, confirmations) uses the formal plural verb form implicit in Lithuanian ("Ar tikrai norite...?" = "Are you sure you want...?"), which is the standard neutral/formal register for software independent of an explicit "Jūs" pronoun.
- Commands, buttons, and menu entries use the infinitive/noun form described in section 2, which carries no formality marking and is the Lithuanian software-industry norm; do not switch to informal "tu" imperative forms (eg avoid "Įrašyk").
- Error/status/debug messages (developer-facing, LOW priority) are impersonal statements, not addressed to a "you"; keep them terse and impersonal as in the existing catalog (eg "nepavyko", "trūksta", "netinkamas").

## 4. NEC2 electromagnetic-simulation domain mapping

- current (electrical) → srovė / srovės (never "dabartinis/dabartis" = present-time sense)
- charge (electrical) → krūvis / krūviai (never "kaina/mokestis" = billing sense)
- ground / ground plane → įžeminimas / atspindys nuo žemės, keep "Groundplane"-style compounds untranslated when the source itself keeps "ground plane" as a proper term; do not use "žemė" (soil) alone
- gain → stiprinimas (never "pelnas" = profit)
- load (impedance) → apkrova (never "krūvis", which is reserved for electrical charge, to avoid lexicon collision)
- wire → laidas (the NEC2 antenna wire/segment conductor; established across the catalog); "vielinis karkasas" is kept only for the distinct "wireframe" 3D-render compound, not for the wire element itself
- excitation → sužadinimas (never emotional sense)
- pattern (radiation) → diagrama / spinduliuotės diagrama (never "šablonas" = template sense)
- impedance → varža
- Disambiguation is achieved by selecting the correct technical sense only; do not append an explanatory qualifier (eg "elektrinė") that is absent from the English source, per frame.md Disambiguation guidance.
