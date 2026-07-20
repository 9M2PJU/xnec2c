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
| polarity (sign) | قطبیت | electrical/color-sign polarity (positive/negative); distinct from پولاریزیشن (EM wave polarization); confirmed against pre-existing obsolete entry "Instantaneous Polarity" -> "سمدستي قطبیت" |
| amplitude | امپلیتیوډ | transliterated noun, consistent with فریکونسي/امپیدنس/ولتاژ pattern |
| hue | رنګ | no distinct Pashto word from "color"; program context (color projection/scale) disambiguates |
| brightness | روڼتیا | established throughout catalog (not "transparency") |
| envelope (signal) | پوښ | eg چوکۍ-پوښ = peak-envelope |
| overlay (UI layer toggle) | لایه | distinct from پوښ (envelope) and پوښنه (verification/check, unrelated existing sense) |
| node / antinode | نوډ / انټي‌نوډ | standing-wave zero/peak points; "null" in this sense = نوډ, not صفر |
| standing / traveling (wave) | ولاړه / تلونکې | VSWR-type wave motion; do not reuse تلپاتې (persistent/eternal) for "standing" |
| static / animated (category) | ثابت / متحرک | paired combobox/section headers; متحرک only for this pairing, not the noun "animation" (انیمیشن) |
| far-field contribution | د لرې ساحې ونډه | لرې ساحه = far field (pairs with existing نږدې ساحه = near field); ونډه = contribution/share |
| comet (visual overlay) | کامېټ | transliterated, parallel to Power/Asinh/Reinhard/Sigmoid scale-family names kept in Latin script |
| encoding (hue/brightness) | کوډ کول | eg د رنګ کوډ کول = hue encoding |

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
- "Polarity" (animate.glade:97): a fuzzy entry carried a stale copy of the
  _پولاریزیشن (Polarization) menu-hotkey translation; polarity (sign) and
  polarization (EM wave) are distinct concepts, split to قطبیت vs پولاریزیشن;
  resolved.
- "Current + Charge" / "Far-field Contribution" (animate.glade combobox
  entries): fuzzy entries carried stale copies of unrelated strings ("Current
  Source", "Near Field Animation"); retranslated from the current msgid;
  resolved.
- "null" (standing-wave/current sense): three animate.glade projection/overlay
  tooltips rendered current "nulls" as صفر ټکي (zero points) while the
  Nodes/antinodes overlay used نوډ; normalized the standing-wave sense to نوډ
  everywhere ("floored so nulls stay visible", "near-null currents", "nulls
  draw thin"); genuine numeric/geometric zeros (zero wire radius, zero-length
  line, division by zero, zero bytes) remain صفر; resolved.
- unhandled hue/brightness encoding and invalid palette-kind/color-projection
  diagnostics (chroma.c, color_palette.c): three distinct c-format messages
  shared one copy-pasted msgstr ("ناسم رنګ پروجیکشن %d، مقدار کارول کیږي" /
  "ناکنترول شوی رنګ پروجیکشن %d"); each now translated from its own msgid;
  resolved.

## Disambiguation
- Domain context (EM simulator) disambiguates "current" (electrical) vs any
  temporal sense; do not add qualifiers absent from the English source.
- "charge" (چارج) and "load" (بار) are kept as distinct words: charge is the
  EM quantity (View Charges, Currents/Charges), load is LD-card loading
  (بارول = to load). Do not merge them; the shared بار would collide with the
  loading vocabulary.
