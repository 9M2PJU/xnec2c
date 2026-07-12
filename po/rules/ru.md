# ru translation rules

## Character set / symbology
- Cyrillic script, UTF-8. Quotes use «guillemets» when quoting UI text inside a sentence.
- Numeric format specifiers (%d, %f, etc.) are runtime values; do not localize decimal separators.
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

## Disambiguation
- Drop the specific technical qualifier ("электрический") when program context (electromagnetic simulator) makes the sense unambiguous; do not add qualifiers absent from the English source.
