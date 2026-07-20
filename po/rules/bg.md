# bg translation rules

## 1. Native character set, symbology, expectations

- Script: Bulgarian Cyrillic (33 letters; includes ъ, ь, щ, ю, я; excludes Russian-only
  letters ё, ы, э). Full uppercase/lowercase distinction exists and is used normally in
  running text and headers.
- GTK accelerators (`_X`) are placed on a Cyrillic letter of the Bulgarian word, matching
  existing catalog practice (eg `_Мигновена`, `_Пикова стойност`). Choose a letter that
  avoids collision with other accelerators in the same menu/dialog.
- Units and acronyms stay Latin/unlocalized per doc/TRANSLATING.md: dBi, MHz, Ω, VSWR,
  NEC2 card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), file extensions
  (.nec, .csv, .s1p, .s2p, .png), format specifiers (%s, %d, %f, %c, %%).
- Decimal separator in literal example numbers embedded in prose (eg "γ = 0.5") uses the
  Bulgarian comma convention: "γ = 0,5". This does not apply to format specifiers
  themselves, which are runtime placeholders, not literal numbers.

## 2. Technical program-interface writing standards

- Menu items, checkbox/radio labels, and section headers: verbal-noun or plain noun
  phrases (eg "Мащабиране" = Scaling, "Възбуждане" = Excitation, "Геометрия" = Geometry),
  matching established catalog usage rather than full imperative sentences.
- Buttons and direct commands: short imperative verb forms (eg "Запази", "Отвори"),
  consistent with existing entries.
- Tooltips and status/error messages: full, grammatically complete sentences with
  standard punctuation and no informal contractions.
- Error/debug messages that stay developer-facing (per TRANSLATING.md LOW priority
  category, and pr_debug-style internal strings) keep concise, neutral technical
  register; still full Bulgarian sentences, no slang.

## 3. Formality and informality mapping

- Direct address to the user (dialogs asking a question, confirmation prompts) uses the
  formal "Вие"/"Ви", capitalized when it is a direct polite address, matching formal
  register used in Bulgarian professional/engineering software.
- Never use the informal "ти" form anywhere in the interface.
- Imperative verb mood on buttons/commands is a UI convention, not an informality
  marker; it coexists with formal address elsewhere (same pattern as German "Sie" +
  imperative buttons).

Sections 1-3 above are disjoint: 1 covers script/symbols, 2 covers sentence/phrase
construction by UI element type, 3 covers pronoun/address formality only.

## 4. NEC2 electromagnetic-simulator domain mapping (established in this catalog)

| English | Bulgarian | Notes |
|---|---|---|
| current (electrical) | Ток / Токове | never "present time" sense |
| charge (electrical) | Заряд / Заряди | never "billing" sense |
| impedance | Импеданс | standard EE loanword; not "съпротивление" (resistance) |
| impedance magnitude \|Z\| | модул / мод | modulus of the complex impedance; never "амплитуда" (signal amplitude) or "магнитуда"; keep short "мод" in terse "мод/фаза" labels |
| amplitude (oscillating field/current) | амплитуда | peak/instantaneous magnitude of a sinusoidal quantity; distinct from impedance modulus |
| wire / conductor | Проводник | NEC2 geometry element |
| geometry | Геометрия | |
| excitation | Възбуждане | EM energy input, not emotional state |
| radiation pattern | Диаграма на излъчване | |
| scaling / scale | Мащаб / Мащабиране | |
| gain | Усилване | antenna directivity ratio, not profit |
| ground (RF reference) | земя / маса | electrical ground plane sense; not soil |
| phase | Фаза | |
| animation | Анимация | |
| color / color scale | Цвят / Цветов(а) | |
| ground plane | земна равнина | always "земна равнина"; never "заземяваща равнина" |
| tapered wire | конусен проводник | adjective "конусен"; taper noun = "конусност" |
| reflect (GX operation) | Отразяване | verbal noun, matches Move=Преместване / Scale=Мащабиране |
| patch | патч / патчове | NEC2 surface patch (SP/SM); not "пач" |
| viewer | наблюдател | Viewer gain/direction |
| net gain | нетно усилване | consistent "нетно" |
| far field | далечно поле | symmetric with established "близко поле" (near field); "far-field contribution" = "принос на далечното поле" |
| ramp (color transfer noun) | градация | eg "Rainbow ramp" → "Дъгова градация"; "invalid palette kind, using ramp" → "...използва се градация" |
| palette kind | вид палитра | distinct from "цветова проекция" (color projection) and "цветово семейство" (scale/tone family) |
| color tone (scale/transfer family, src/color/color_tone.c) | цветово семейство | "тон" is reserved for hue (see below); avoid "цветови тон" to prevent collision |
| hue | тон | eg "Phase Hue" = "Фазов тон"; do not reuse "тон" for the color_tone.c "family" concept |
| standing wave / traveling wave | стояща вълна / бягаща вълна | "Standing/Traveling" label → "Стояща/Бягаща" |
| comet (overlay) | комета | visual trail effect on animated wave crest; not "геометрия" |
| contribution | принос | eg far-field contribution |
| polarity (instantaneous sign encoding) | Полярност | sign of the displayed quantity (cold negative / hot positive); never "Поляризация" |
| polarization (antenna/wave) | поляризация | EM wave/antenna polarization (Horizontal/Vertical/Circular, polarization axis); distinct from polarity |

Axis coordinates X/Y/Z stay uppercase in messages even when the English source
lowercases them (eg "step size limited at z=" → "...при Z="), per the frame's
axis-casing rule and the majority catalog usage.

Disambiguation follows doc/TRANSLATING.md: pick the correct technical sense from the
table without adding qualifiers absent from the English source (eg "Токове" alone for
"View Currents", not "Електрически токове").

## 5. Status

Filled per Phase 1 of po/prompt/frame.md against the existing po/bg.po catalog.
