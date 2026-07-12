# mk translation rules

## Script and formatting
- Macedonian Cyrillic (distinct letters: Ѓ ѓ, Ѕ ѕ, Ј ј, Љ љ, Њ њ, Ќ ќ, Џ џ).
- Keep numeric/unit tokens and format specifiers unchanged and LTR: `%s %d %f %c %%`, `dBi`, `MHz`, `VSWR`, `Ω`, `dB`, `φ=0`.
- NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), file extensions (.nec, .csv, .s1p, .s2p, .png), and proper nouns (Smith, S11, S-parameters, NEC2) stay untranslated.

## Register
- Two imperative registers, applied consistently by string kind:
  - Full-sentence instructions/tooltips: formal-neutral 2nd person plural imperative (-ете, -ајте), e.g. "Изберете", "Прикажете ја", "Анимирајте ја", "Овозможете го", "Вратете ги". No explicit "Вие" pronoun needed; this is the established UI convention.
  - Short button/menu action labels: terse singular imperative (-и, -ај), e.g. "Ресетирај", "Зачувај", "Избери", "Прикажи", "Избриши", "Скалирај". This is the established catalog convention for labels.
- Sentence case for labels; no forced capitalization.
- Disambiguation: use plain domain term without adding "електричен/електрична" qualifier unless the English source itself qualifies it (eg "струи" for "Currents", not "електрични струи").

## Established lexicon (reuse exactly)
| English | Macedonian |
|---|---|
| gain (antenna) | добиток |
| viewer | прегледувач |
| current(s) | струја / струи |
| charge(s) | полнеж / полнежи |
| wire(s) | жица / жици |
| patch(es) / surface | плочка / површина / површини |
| radiation pattern | дијаграм на зрачење |
| polarization | поларизација |
| phase | фаза |
| widget | виџет |
| card (NEC2) | картичка |
| scale | скала / скалирај |
| projection | проекција |
| optimizer | оптимизатор |
| flow direction | насока на тек |
| wireframe | жичен приказ |
| peak value/magnitude | врвна вредност / врвна амплитуда |
| reference phase | референтна фаза |
| instantaneous (φ=0) | моментален (φ=0) |
| geometry | геометрија |
| validation | валидација |
| magnitude (field/current) | амплитуда |
| magnitude/modulus (impedance \|Z\|) | модул (eg "Z-модул", "мод/фаза") |

## New lexicon coined this pass
| English | Macedonian | Rationale |
|---|---|---|
| color projection | проекција на боја | parallels existing "проекција" |
| scale family | фамилија на скали | "family" = фамилија, established loanword |
| gamma (exponent) | гама | standard EE/photography transliteration |
| contrast (steepness) | контраст | established loanword |
| knee (soft-knee point) | колено | literal EE term for bend point |
| compression (dB) | компресија | established loanword |
| softening | омекнување | native formation from "мек" (soft) |
| dynamic range | динамички опсег | standard EE term |
| brightness | осветленост | standard term |
| Reinhard / Sigmoid / Asinh / Power / μ-law | left as proper/technical names, only UI verb text translated around them | algorithm names, not translated (cf. "Smith" chart precedent) |
