# ru translation rules

## Character set / symbology
- Cyrillic script, UTF-8. Quotes use «guillemets» when quoting UI text inside a sentence.
- Numeric format specifiers (%d, %f, etc.) are runtime values; do not localize decimal separators.
- Literal decimals in prose/formulas (e.g. `γ = 0.5`, `48.16 dB`) and spinbutton default values (`50.00`, `0.0100`) keep the dot separator for catalog-wide consistency with formula and default notation (consistency priority outranks locale-decimal form here); do not switch these to comma.
- No case-folding ambiguity; Cyrillic has full upper/lower case.

## Technical writing style
- Commands/buttons: imperative/infinitive mood (e.g. "Сохранить", "Открыть"), GOST-style technical register.
- Labels/menus: sentence case (capitalize only first word), not English Title Case.
- Avoid literal English calques; prefer natural Russian engineering phrasing.
- NEC2 mnemonics (GW, EX, LD, FR, RP, GE, EN), unit symbols (MHz, dBi, Ω, VSWR), and file extensions stay in Latin form, per existing catalog precedent — do not substitute Cyrillic abbreviations (e.g. КСВ) unless the catalog already established that substitution at that entry.

## Formality
- Formal «Вы» (capitalized in direct address) throughout; never «ты».

## NEC2 domain lexicon (established in this catalog)
- ток / токи — (electrical) current
- заряд / заряды — (electrical) charge
- патч — patch (surface patch, transliterated)
- тег — tag
- сегмент — segment
- диаграмма направленности — radiation pattern
- возбуждение — excitation
- нагрузка — load (impedance sense)
- усиление — gain
- земля / плоскость земли — ground (electrical), never soil
- структура — structure (the antenna model/geometry), never «конструкция» (catalog precedent: "Structure Geometry" → «Геометрия структуры»)
- масштаб — scale
- каркасный — wireframe
- пиковое значение — peak value
- мгновенное (φ=0) — instantaneous (φ=0)
- опорная фаза — reference phase
- направление потока — flow direction
- визуализация — visualization
- полярность — polarity (sign polarity: cold/negative vs hot/positive), never «поляризация» (wave polarization — different EM concept)
- мгновенное — instantaneous; add the "(φ=0)" qualifier only where the English source itself carries it
- вклад в дальнее поле — far-field contribution
- узлы/пучности — nodes/antinodes (standing-wave sense)
- семейство шкал — scale/tone family (color_tone.c transfer curves: Power, Log, μ-law, Reinhard, Sigmoid, Asinh)
- цветовая проекция — color/hue projection (chroma.c hue-source selection, distinct from "семейство шкал")
- статично / анимация — static / animated (as category or read-mode qualifiers)

## Disambiguation
- Drop the specific technical qualifier ("электрический") when program context (electromagnetic simulator) makes the sense unambiguous; do not add qualifiers absent from the English source.
