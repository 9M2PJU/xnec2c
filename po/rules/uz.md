# uz translation rules

## 1. Character set / symbology
- Latin script (post-1995 Uzbek Latin alphabet). Digraphs: `sh`, `ch`, `ng`, `oʻ`, `gʻ`.
- The apostrophe letter `ʻ` is U+02BB (MODIFIER LETTER TURNED COMMA) — use this glyph exclusively (never `'` U+0027 or `’` U+2019) in words like `koʻrish`, `oʻqi`, `qutblanish`.
- Standard Latin uppercase/lowercase rules; X/Y/Z axis names stay uppercase.
- Technical symbols/units (%, MHz, dBi, Ω, VSWR, dB) stay untranslated.
- Decimal/number formatting inside format specifiers is left to runtime; never translate digits or specifier content.

## 2. Technical UI writing standards
- Loan/transliterated technical nouns are preferred over invented coinages, matching established GNOME/KDE Uzbek practice already present in this catalog:
  `vidjet` (widget), `fayl` (file), `sozlama`/`sozlash` (setting/configure），
  `xato`/`xatosi` (error), `tugma` (button), `oyna` (window), `karta`/`kartasi` (NEC2 card).
- Sentences: concise, verb-final; imperative mood for actions/buttons, noun phrases for labels and titles.

## 3. Formality
- Uzbek carries formality through the verb ending, not pronoun choice (pronouns are normally omitted).
- Use the formal `siz`-register imperative ending (`-ing`/`-ingiz`), e.g. `tanlang`, `yoqing`, `foydalaning` — never the bare informal `sen`-register stem.
- Apply uniformly across all buttons, tooltips, and instructions.

## 4. NEC2 domain glossary (Uzbek)
| English | Uzbek | Notes |
|---|---|---|
| current (electrical) | tok | never `joriy`/`hozirgi` (time sense) |
| charge | zaryad | electrical charge |
| gain | kuchaytirish | antenna directivity, not profit |
| polarization | qutblanish | |
| field | maydon | |
| radiation | nurlanish | |
| radiation pattern | nurlanish diagrammasi | |
| frequency | chastota | |
| geometry | geometriya | |
| direction | yoʻnalish | |
| magnitude | kattalik | |
| surface | sirt | |
| axis | oʻq / oʻqi | |
| reference phase | tayanch faza | |
| instantaneous | Momentan | |
| color | rang | |
| scale | masshtab | never `shkala`; also `logarifmik masshtab` for "log scale" |
| logarithmic | logarifmik | never `logaritmik`; Uzbek base is `logarifm` |
| viewer gain | koʻruvchi kuchaytirish | do not add `yoʻnalishidagi` (direction) absent from "Viewer Gain"; that qualifier belongs only to "Gain in Viewer Direction" |
| projection | proeksiya | |
| flow | oqim | |
| patch | patch | NEC2 term, untranslated |
| segment | segment | NEC2 term, untranslated |
| tag | tag | NEC2 term, untranslated |
| widget | vidjet | |
| error | xato / xatosi | |
| card (NEC2) | karta / kartasi | |
| feedpoint | boshlangʻich nuqta | excitation feed point |
| excitation | qoʻzgʻatish | |
| port | port | |
| brightness | yorqinlik | |
| contrast | kontrast | |
| gamma | gamma | |
| softening | yumshatish | |
| compression | siqish | |
| knee | tizza nuqtasi | soft-compressor knee |
| wireframe | sim karkas | |
| structure viewer | tuzilma koʻruvchi | |
| validation | tekshiruv | |
| theme | mavzu | UI theme |
