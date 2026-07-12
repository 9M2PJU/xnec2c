# de translation rules

## 1. Native character set, symbology, expectations
- Latin alphabet plus umlauts (Ä/Ö/Ü, ä/ö/ü) and Eszett (ß); use standard (not Swiss) German orthography, so ß is retained (not "ss").
- Decimal separator is the comma ("50,0"), thousands separator is the period; but numeric values in this catalog come from runtime `%f`/`%d` format specifiers, not literal text, so no msgstr text needs comma substitution unless a literal example number appears in the source string.
- Straight ASCII quotes `'...'` are used throughout the existing catalog for widget-name quoting; keep this convention rather than introducing „…" typographic quotes, for consistency with existing entries.
- Unit symbols stay unchanged: MHz, dBi, Ω, VSWR, %, degrees.

## 2. Writing standards for a technical program interface
- All German nouns are capitalized, including inside compound technical nouns (Strahlungsdiagramm, Ladungsverteilung, Speisepunkt).
- Prefer closed compounds (Kompositum) over hyphenated or multi-word phrases where German normally fuses them: "Segmentverbindungsfehler", not "Segment Verbindungs Fehler".
- Menu items, buttons, and labels use noun/infinitive phrases (title-style only for the first word and embedded nouns), not English-style capitalize-every-word.
- Established loanwords used in German amateur-radio/engineering practice may stay as-is when no accepted German equivalent is in common technical use (e.g. "Groundplane" alongside "Masseebene"); prefer the loanword when it is the dominant term in German ham-radio literature.
- NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), file extensions (.nec, .csv, .s1p, .s2p, .png), and debug/developer-only strings (pr_debug-level) stay in English per doc/TRANSLATING.md.

## 3. Formality and informality mapping
- Use formal "Sie" address and matching verb forms wherever a pronoun is grammatically required (e.g. "Wählen Sie", "Klicken Sie").
- Prefer neutral infinitive/noun-phrase forms for commands and short labels ("Speichern", "Öffnen", "Zurücksetzen") — this is the German software-UI norm and sidesteps Du/Sie choice entirely for terse UI strings.
- Never use informal "Du" forms; this is professional engineering software for a technical, formal audience (DARC/VDE-aligned register).
- Sections 1-3 do not overlap: §1 is script/punctuation/units, §2 is capitalization/compounding/lexical choice, §3 is pronoun/mood only.

## 4. Domain mapping (NEC2 electromagnetic simulator, German culture)
- Strom = electrical current (Ampere), never "aktuell/gegenwärtig" (temporal "current").
- Ladung = electrical charge (Coulomb), never billing/cost sense.
- Masse / Erde / Erdungsebene = electrical ground/ground-plane reference, never soil; "Groundplane" (loanword) acceptable per §2.
- Draht = thin conductor (antenna wire), never "Kabel" (cable/cord).
- Gewinn = antenna directivity ratio (dB), never "Profit/Gewinnspanne".
- Muster / Diagramm: radiation pattern = "Strahlungsdiagramm", never "Vorlage/Design".
- Anregung = electromagnetic excitation/source, never emotional excitation.
- Last / Impedanz = electrical load/impedance, never physical weight.
- Radial = "Radial" (horizontal ground-plane wire), not related to "radius" adjective use.
- Speisepunkt = feedpoint; Anregungsport = excitation port.
- VSWR, S-Parameter, Impedanz, dBi, MHz, Ω, NEC2 kept per doc/TRANSLATING.md never-translate list.
- Disambiguation is implicit from the EM-simulator context; do not append qualifiers like "elektrisch" that are absent in the English source (e.g. "Ströme betrachten", not "Elektrische Ströme betrachten").

## 5. Established lexicon (grows during translation)
| English | German | Notes |
|---|---|---|
| current (electrical) | Strom | never "aktuell" |
| charge | Ladung | electrical charge |
| ground plane | Groundplane / Masseebene | loanword accepted |
| wire | Draht | not Kabel |
| gain | Gewinn | antenna directivity |
| pattern (radiation) | Strahlungsdiagramm / Muster | context |
| excitation | Anregung | EM energy input |
| load / impedance | Last / Impedanz | electrical |
| feedpoint | Speisepunkt | |
| projection | Projektion | color/geometry projection |
| scale | Skalieren / Skala | verb vs noun per context |
| geometry | Geometrie | |
| widget | Widget | loanword, kept |
| animate/animation | animieren / Animation | |
| flow direction | Flussrichtung | |
| color | Farbe | |
| brightness | Helligkeit | |
| contrast | Kontrast | |
| compression | Kompression | |
| knee (curve) | Knie | rational soft-compressor term |
| dynamic range | Dynamikbereich | |
| validation | Validierung | |
| default | Standard / Vorgabe | |
| power (transfer/scale family) | Potenz | math power-law transfer, not electrical "Leistung" |
| color scale/projection | Farbskala / Farbprojektion | |
| softening | Weichheit | tone-mapping softening knee |
| compression | Kompression | dynamic-range compression |
| radio button | Radiobutton | loanword, developer-facing strings |
| combo box | Combobox | loanword, developer-facing strings |
| radio menu item | Radio-Menüeintrag | |
| theme | Theme | loanword, kept for UI/dev messages; never "Thema" (=topic/subject); color theme = closed compound "Farbtheme" |
| port (excitation) | Port | loanword, RF/S-parameter convention |
| F/B (front/back ratio) | V/R-Verhältnis | Vor/Rück; symbol kept as V/R |
| Smith Chart | Smith-Diagramm | proper-name compound |
| scale family | Skalenfamilie | color transfer family |
| net gain | Nettogewinn | closed compound |
| viewer gain | Betrachtergewinn | gain in viewer direction |
| flow direction | Flussrichtung | patch current flow |

## 6. Card-label conventions
- NEC2 card dialog/editor titles keep the English designator "(XX Card)" as a fixed label (e.g. "Strahlungsdiagramm (RP Card)", "Anregungsbefehl (EX Card)"); this is the consistent title-widget convention across the geometry/command editor.
- Running prose (status/error messages) instead uses the hyphenated German "XX-Karte" (e.g. "RP-Karte", "GW-Karte", "Doppelte FR-Karte ... übersprungen").
- Keep each register internally consistent; do not cross-convert one style into the other.
