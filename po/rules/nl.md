# nl translation rules

## 1. Native character set, symbology, expectations
- Latin script; diacritics limited to trema (ë, ï, ü) marking a separately pronounced vowel in a digraph (eg "geïnstalleerd", "µ" stays as the Greek mu symbol, not a Dutch letter).
- Digraph "ij" capitalizes as a unit at the start of a sentence/title ("IJs"), not relevant mid-sentence for this catalog.
- Straight quotes are standard in UI strings; no special Dutch quotation glyphs required for short labels/tooltips.
- Decimal comma is the Dutch locale convention (eg "50,0" not "50.0"), but literal numeric example text inside msgids is not altered, only prose numerals in msgstr when spelled out.
- Units, NEC2 mnemonics, file extensions stay unchanged per doc/TRANSLATING.md (MHz, dBi, Ω, VSWR, GW, EX, LD, .nec, .csv, .s1p, .s2p).

## 2. Technical interface writing standards (Dutch)
- Menu items and buttons: infinitive or noun form, not conjugated imperative with subject (eg "Opslaan" not "Sla op je bestand"); this catalog already follows that pattern ("Frequentielus starten bij opstarten").
- Sentence case: capitalize only the first word and proper nouns/acronyms; Dutch does not capitalize every noun (unlike German).
- Compound technical nouns are joined without spaces or hyphens where standard Dutch permits (eg "Stroomvisualisatie", "Frequentiestap", "Kleurprojectie"), matching existing catalog entries.
- Established borrowed English IT/engineering terms remain untranslated where Dutch engineering practice keeps them (eg "reset", "scale"/"schaal" both seen; prefer native "Schalen" already established), but NEC2/RF domain terms follow doc/TRANSLATING.md's disambiguation table.
- Tooltips/help text: full sentences, standard Dutch punctuation, formal register (see §3).

## 3. Formality mapping
- Professional/engineering desktop software in Dutch avoids direct second-person address where possible, using infinitive/imperative-neutral phrasing for actions ("Herstel de kleurprojectie..." reads as an infinitive-style instruction, not "jij"-directed).
- Where direct address is unavoidable (rare in this catalog), use formal "u" register, never informal "je/jij" — consistent with professional engineering-tool conventions.
- Sections 1-3 do not overlap: §1 is script/glyph mechanics, §2 is interface grammar/capitalization/compounding, §3 is only who is addressed and how.

## 4. NEC2/xnec2c domain mapping (established in this catalog)
- current → "Stroom" (electrical current, Amperes) — never "huidig"/"actueel" (temporal "current").
- charge → "Lading"/"Ladingen" (electrical charge) — never "kosten"/"lading" as cargo/billing sense.
- ground/ground plane → domain RF sense; existing catalog uses "grond" only in NEC2 card-name context ("Golftype en grond (I1)"); no qualifier added beyond source.
- gain → "Versterking" (antenna directivity, dB) — never "winst" (profit).
- pattern → "Patroon" (radiation pattern) — never "ontwerp"/"sjabloon".
- excitation → "Excitatie" — never emotional sense.
- load/impedance → "Impedantie" — never "gewicht"/"last".
- wire/segment/patch/tag → "Draad"/"Segment"/"Patch"/"Tag", NEC2 geometry terms kept per doc/TRANSLATING.md (segment, patch, tag never translated to generic synonyms).
- Do not append "elektrische" before Stroom/Lading/etc.; program context (an EM simulator) already disambiguates, matching frame.md's Afrikaans/German/Spanish examples.

## Established lexicon (consistency anchor, from existing entries)
- Stromen = Currents (view/menu term)
- Ladingen = Charges
- Stroomvisualisatie = current visualization
- Stroomrichting = flow direction
- Impedantie = impedance
- Frequentielus = frequency loop
- Stralingspatroon = radiation pattern
- Draadframe = wireframe
- Piekwaarde = peak value
- Referentiefase = reference phase
- Excitatietype = excitation type
- Geometrie = geometry
- Schalen = scale(s)
- Fase = phase
- Kleurprojectie = color projection
- Schaalfamilie = scale family
- Vermogen = Power (transfer family; not "Lineair vermogen" unless source says so)
- Sigmoïde = Sigmoid (transfer family curve name)
- Asinh, Reinhard, μ-law = kept untranslated (named/standard algorithm terms)
- Bereik = Range (dB range control)
- Verzachting/Verzachtingsknik = Softening/softening knee
- Compressie = Compression
- Knik = Knee (rational soft-compressor knee)
- Contrast/Contraststeilheid = Contrast/contrast steepness
- Herstellen = Reset (button)
- Weergave = Display (section heading)
- Kleur = Color (heading), Kleurschaal = Color Scale (menu heading)
- Geen (geometrie) = None (geometry) coloring option
- Momentaan/Momentane amplitude/Momentane polariteit = Instantaneous/Instantaneous Magnitude/Instantaneous Polarity
- Fasekleur = Phase Hue
- Poort = Port
- (geanimeerd) / (statisch) = (animated) / (static) qualifiers, always kept, no underscore mnemonic on non-menu combo items
- Patches = Patches (NEC2 surface-patch term, plural kept as English loanword)
- Totaal veld = Total field
- Validatieboom = Validation Tree
- Opdrachtkaart / Geometriekaart = Command card / Geometry card (NEC2 input-file card sections)
- Debug/dev pr_* messages: translate surrounding English words, keep function/identifier names (eg config_widget_lookup, mem-report, rc_config_field_size) and all format specifiers verbatim.
- Hoofdas / Nevenas = major axis / minor axis (polarization ellipse); pair consistently, never "Kleine as" for minor.
- magnitude → "grootte" for a static/modulus quantity (impedance |Z| = "Z-grootte", "Z grootte / fase"; vector "grootte en richting"); never "amplitude" for impedance modulus. Reserve "amplitude" for the oscillating-field color-projection family only (Momentane amplitude, Piekamplitude, "using amplitude").
- F/B (front-to-back ratio) → "V/A" (voor-achterverhouding); apply everywhere including compact chart-list abbreviations, never leave standalone "F/B" untranslated once "V/A-verhouding" is the chosen term.
- oppervlaktepatches = surface patches; use the combining prefix "oppervlakte-" for "surface" in compounds (oppervlaktestromen, oppervlaktegolf), matching standard Dutch technical usage; reserve standalone "oppervlak" for the head-noun sense (Patchoppervlak, draw-style "oppervlak", Cairo-oppervlak); eg "Surface Sub-division" → "Oppervlakteonderverdeling" (modifier compound), not "Oppervlakonderverdeling".
- Address: formal "u" register only; never informal "je/jij/jouw" (eg "voordat u benchmarks uitvoert").
- Lowercase math/coordinate variable "z" (eg "step size limited at z=", "Hankel not valid for z = 0") stays lowercase; do not capitalize to the axis Z. The X/Y/Z-uppercase rule applies to axis labels, not lowercase source variables.
- Animated/Static color-projection group headers (combo/radio category labels, not buttons) → "Geanimeerd"/"Statisch", no underscore mnemonic, matching the existing "(geanimeerd)/(statisch)" qualifier convention.
- Polarity (instantaneous sign of a displayed quantity) → "Polariteit", no mnemonic when it is a combo/radio item name, not a menu button.
- "Current + Charge" (paired-quantity projection name) → "Stroom + Lading", preserving the "+" join.
- far field → "verre veld", hyphenated in compounds matching the existing "nabije-veld" pattern (eg "Verre-veldbijdrage" for "Far-field Contribution", parallel to "Nabije-veldanimatie" for "Near Field Animation").
- Nodes/antinodes (standing-wave) → "Knopen/buiken" (classic Dutch standing-wave physics terms), not a literal transliteration; "current nodes/antinodes" → "stroomknopen/-buiken".
- Comet (moving-crest highlight overlay) → "Komeet"/"kometenkop" for "comet head".
- ramp/gradient (color-transfer or palette-kind sense, eg "Rainbow ramp", "Color gradient", "using ramp") → "verloop" ("Regenboogverloop", "Kleurverloop"); do not confuse with "Bereik" (dB range control).
- "Brightness floor" → "Helderheidsondergrens" (floor = "ondergrens" throughout, eg "met een ondergrens zodat nullen zichtbaar blijven").
- "Overlays:" kept as the English loanword (established graphics/rendering-software term in Dutch).
- palette kind → "paletsoort"; hue encoding / brightness encoding (internal chroma dev messages) → "tintcodering" / "helderheidscodering"; never reuse "kleurprojectie" for these distinct internal enums.
- "color tone" (the scale/transfer-family enum in src/color/color_tone.c) → "schaalfamilie", matching the established "Schaalfamilie" UI term; do not introduce a separate "kleurfamilie" synonym.
- Compound "X vectoren" (eg Poynting-vermogensstroomvectoren) joins without an internal space; Dutch compounds are not split before the head noun.
