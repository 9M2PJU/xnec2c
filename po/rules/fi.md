# fi translation rules

## 1. Script & symbology
- Latin alphabet plus ä, ö (å only in names/Swedish loanwords).
- Decimal separator is comma (`50,0`), thousands separator is a space; keep numerals/format specifiers untouched regardless.
- Typographic quotes: `”...”` (identical glyph both sides), not „...“ or «...».
- No noun capitalization (unlike German); sentence case throughout.
- NEC2 mnemonics, unit symbols (MHz, dBi, Ω, VSWR) and format specifiers (%s/%d/%f/%c/%%) stay LTR/unchanged and in source order.

## 2. Technical writing standards
- Commands/buttons/menu items: 2nd-person singular imperative (`Avaa`, `Tallenna`, `Valitse`), matching GNOME/KDE/Debian fi conventions.
- Descriptive/tooltip/help text: passive or impersonal constructions (`voidaan`, `voit`) rather than a stated pronoun.
- Prefer a single agglutinative compound over a multi-word phrase when a natural one exists.

## 3. Formality mapping
- Finnish has no strong T–V distinction; formal `Te` reads archaic/stilted in engineering software.
- Use the informal register implicit in the imperative/verb form (never `Te`).

## 4. NEC2 domain mapping (established lexicon)
- current → virta
- charge → varaus
- phase → vaihe
- excitation / excitation type → herätys / Herätteen tyyppi
- feedpoint → herätepiste
- wireframe → rautalankamalli
- magnitude → suuruus (never itseisarvo; keep one term catalog-wide)
- peak value → huippuarvo
- total field → kokonaiskenttä
- instantaneous (φ=0) → hetkellinen (φ=0)
- scale → skaalaa / skaala
- polarization axis → polarisaatioakseli
- flow direction → virtaussuunta
- reference phase → referenssivaihe
- gain → vahvistus
- impedance → impedanssi
- ground plane / radials → maataso / vastapainot
- patch (NEC2 geometry term, never translated per doc/TRANSLATING.md) → patch, plural patchit
- segment (never translated) → segmentti/segmentit
- geometry → geometria
- color projection → väriprojektio
- scale family → skaalaperhe
- color scale → väriskaala
- gamma → gamma
- softening knee → pehmennyspolvi
- compression → kompressio
- contrast → kontrasti
- validation → validointi
