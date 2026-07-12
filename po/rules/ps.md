# ps translation rules

## Script and symbology
- Perso-Arabic script, right-to-left, no uppercase/lowercase concept.
- Pashto-specific letters preserved: ټ ډ ړ ږ ښ ګ ڼ ی ې.
- Numbers, format specifiers (`%s %d %f %c %%`), unit symbols (MHz, dBi, Ω, VSWR),
  and NEC2 mnemonics (GW, EX, LD, FR, RP, GE, EN) stay Western/LTR, embedded
  unchanged in the RTL sentence.
- Standard Pashto punctuation (، ؛ ؟) in prose sentences; colons and technical
  strings keep ASCII `:` as already used throughout the catalog.

## Register and style
- Passive/impersonal construction (`... کیږي`, `... کیدی شي`) is the established
  idiom for system/status/error messages; keep using it for consistency.
- Polite plural imperative (`وکاروئ`, `وګورئ`) for user instructions and menu
  verbs, not blunt singular imperative.
- No strong formal/informal (T/V) split needed; maintain a neutral professional
  register throughout.

## Established lexicon (reuse exactly; do not introduce synonyms)
| English | Pashto | Notes |
|---|---|---|
| current | جریان | electrical current (Amperes) |
| charge | چارج | EM charge quantity (Coulombs); kept distinct from load |
| load | بار | LD-card loading; verb بارول (to load) |
| field | ساحه | EM field |
| frequency | فریکونسي | |
| impedance | امپیدنس | |
| voltage | ولتاژ | |
| phase | فاز | |
| wire(s) | تار / تارونه | |
| patch(es) | پیچ / پیچونه | |
| geometry | جیومیتري | |
| optimizer | اپتیمایزر | canonical spelling; do not use آپټیمایزر |
| config/settings | ترتیب | |
| widget | ویجیټ | |
| card (NEC2) | کارډ | |
| direction | لوری | |
| gain | ګټه | feminine noun; agree adjectives/participles (ټوله, نورمالیزه شوې, محاسبه شوه); never transliterate as ګین |
| direction | لوری | masculine; never render as لار (that is path/way) |
| polarization | پولاریزیشن | |
| scale | پیمانه | |
| color | رنګ | |
| gamma | ګاما | |
| dynamic range | متحرک ساحه | keep "dB" unit |
| viewer | لیدونکي | agentive noun; oblique لیدونکي, nominative لیدونکی; never ویونکي (that is speaker) |

## Known inconsistencies to correct
- "optimizer": normalized to اپتیمایزر everywhere (was آپټیمایزر); resolved.
- "gain": older glade menu entries used the transliteration ګین; normalized to
  canonical ګټه everywhere with feminine agreement; resolved.
- "direction": gain-direction plot labels used لار (path); normalized to لوری;
  resolved. ("traces" as لارې is a distinct word sense and left as-is.)
- "impedance": stray spelling امپیډنس (retroflex ډ) normalized to امپیدنس;
  resolved.
- "viewer": one help-text entry used ویونکي (speaker); normalized to لیدونکي to
  match every other viewer string; resolved.

## Disambiguation
- Domain context (EM simulator) disambiguates "current" (electrical) vs any
  temporal sense; do not add qualifiers absent from the English source.
- "charge" (چارج) and "load" (بار) are kept as distinct words: charge is the
  EM quantity (View Charges, Currents/Charges), load is LD-card loading
  (بارول = to load). Do not merge them; the shared بار would collide with the
  loading vocabulary.
