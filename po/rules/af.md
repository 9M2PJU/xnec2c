# af translation rules

## 1. Character set and symbology

- Latin script with diacritics: ë, ï, ê, é, è, ô, û, ó, á (eg "geïnisialiseer", "roosterformaat", "geëvalueer").
- Decimal comma, not decimal point: `50.0` renders `50,00`; existing catalog confirms (`resources/xnec2c.glade:4019` → `0,00000`).
- Sentence case only; no German-style noun capitalization. Proper nouns and NEC2 card mnemonics (GW, EX, LD, FR, RP, GE, EN) stay as-is, unchanged and uppercase.
- Unit symbols unchanged: MHz, dBi, Ω, S/m, dB, K.
- Compound technical terms merge as one word where Afrikaans convention allows (eg "segmentverbindingsfout", "geheue-toewysing") or hyphenate at a vowel clash; use hyphen only to prevent ambiguous vowel runs.

## 2. Technical interface writing standards

- Menu items and button labels: imperative/infinitive mood, terse (eg "Stoor", "Skrap Kaart", "Wysig Geometriedata").
- Status and error messages: impersonal, declarative constructions (eg "Fout met lees van rydata"), matching existing catalog precedent.
- No noun-capitalization; German-influenced calques are avoided in favor of native Afrikaans or established engineering terms already in the catalog.
- GTK hotkey underscore precedes the mnemonic letter; choose a letter that avoids collision within the same menu, preferring the first letter of the Afrikaans word over transliterating the English mnemonic.

## 3. Formality mapping

- Afrikaans software localization (translate.org.za precedent, matching this catalog) uses **jy** (informal "you") for direct address in dialogs (eg "Is jy seker jy wil xnec2c verlaat?"), not the formal **u**.
- **u** is reserved for formal correspondence contexts absent from this interface; do not introduce it.
- Most UI strings avoid direct address entirely, using imperative mood or impersonal passive constructions instead (eg "Herstel na _Verstekwaardes", "Kan nie data stoor terwyl frekwensielus loop nie").
- This section governs only formality register; it does not intersect with character set (1) or domain terminology (4).

## 4. NEC2 domain lexicon

Established terms already in the catalog; reuse these exactly, everywhere:

| English | Afrikaans | Notes |
|---|---|---|
| current(s) (electrical) | Strome / stroom | electrical sense only |
| charge(s) | Ladings / lading | electrical sense only |
| gain | Aanwins | antenna directivity |
| ground / ground plane | Grond / Grondvlak | RF ground reference |
| wire | Draad | thin conductor |
| diameter | Diameter / Draaddiameter | loanword canonical; never mix with "deursnee" (eg "Diameter Seg", "Diam. Taps", "Draaddiameter") |
| pattern cut | Patroonsnit / Patroonsnitte | single linking, never double-s "Patroonssnitte" |
| thread (compute) | Draad | same word as wire; program context disambiguates |
| load (electrical) | Belasting | impedance load |
| excitation | Opwekking | EM energy input |
| pattern (radiation) | Patroon / Stralingspatroon | |
| segment | Segment | unchanged |
| patch | Patch | kept untranslated, matches catalog precedent |
| impedance | Impedansie | |
| resistance | Weerstand | |
| reactance | Reaktansie | |
| inductance | Induktansie | |
| capacitance | Kapasitansie | |
| conductivity | Geleidingsvermoë | |
| dielectric | Diëlektries(e) | |
| polarization | Polarisasie | |
| frequency | Frekwensie | |
| wave | Golf | |
| card (NEC2 mnemonic context) | Kaart | eg "GA-kaart" |
| optimization | Optimalisering | process/setting; never the orphan "Optimering"; menu path quotes match label "Optimalisering Instellings" |
| optimizer | Optimeerder | agent noun (thread, tool); distinct from optimization |
| renderer | Tekenaar | eg "as-tekenaar" |
| shader | Skakeerder | |
| allocation (memory) | Toewysing | |
| radiation | Straling | |
| near field | Nabyeveld / Nabye ... veld | |
| animate/animation | Animeer / Animasie | |
| projection | Projeksie | |
| scale (verb/noun) | Skaal / Skaleer | context-dependent |
| brightness | Helderheid | |
| softening / knee | Versagting / Knie | signal-processing sense |
| compression | Kompressie | |
| noise (electronic) | Ruis | not "geraas" (acoustic racket); use for noise temperature everywhere (eg "Ruistemperatuur", "Antennaruistemperatuur") |
| widget (GTK, diagnostics) | Legstuk | consistent across config_widget/callbacks/sy_overrides |
| degrees (freestanding, prose/axis title) | grade | eg "Rad Angle - deg" → "Stralingshoek - grade", "360 degrees" → "360 grade" |
| deg (parenthetical unit tag) | deg | keep verbatim like other unit tags (m), (Hz); eg "Initial Phi (deg)" → "Aanvanklike Phi (deg)" |
| hue | Kleurskakering / Skakering | color-wheel angle; compounds as "Faseskakering" (Phase Hue), "kleurskakeringswiel" (hue wheel) |
| polarity | Polariteit | sign of a quantity (+/−); distinct from polarization — false friend, do not confuse |
| polarization | Polarisasie | EM wave polarization (existing catalog precedent, unchanged) |
| palette | Palet / paletsoort | color palette selection |
| tone (color-transfer curve family) | Toon / kleurtoon | distinct from "kleurfamilie"; "invalid color tone" → "ongeldige kleurtoon" |
| comet (animation overlay) | Komeet | eg "Comet" → "Komeet", "comet head" → "kometkop" |
| standing wave / traveling wave | Staande golf / Reisende golf | short form "Staande/Reisend" for paired UI label |
| far field | Verreveld | mirrors existing "Nabyeveld" (near field) pattern; eg "Far-field Contribution" → "Verreveld-bydrae" |
| animated/static (category header pair) | Geanimeer / Staties | section labels grouping color-projection entries; no hotkey underscore, since these are listbox/combo entries, not buttons |
