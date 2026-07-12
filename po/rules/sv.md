# sv translation rules

## 1. Native character set, symbology, expectations

- Latin alphabet plus å, ä, ö (distinct letters, sort after z).
- Decimal separator is comma (`50,0`), not period, in prose; numeric widgets/format
  specifiers (`%f`, `%d`) are runtime-generated and unaffected — never edit the specifier.
- Thousands separator is a space (`1 000`), not comma or period.
- No special uppercase/lowercase symbology rules beyond standard European casing.
- SI unit symbols (MHz, dB, dBi, Ω) are used unchanged, same as source.

## 2. Technical program interface writing standards

- Concise, direct register; avoid padding words.
- Buttons/menu items/commands use imperative or infinitive verb form
  ("Spara" = Save, "Öppna" = Open), not a conjugated statement.
- Swedish technical prose favors compound nouns (closed, no hyphen), e.g.
  "strömfördelning" (current distribution), "impedansanpassning" (impedance matching),
  matching German-style compounding noted in TRANSLATING.md.
- Loanwords and abbreviations from English/international EE usage are kept as-is
  when no established Swedish equivalent exists in amateur-radio/engineering practice
  (VSWR, Smith-diagram, dB, dBi).

## 3. Formality / informality mapping

- Swedish software culture (post du-reformen) uses **du** almost universally; **ni**
  reads as archaic/distancing and must be avoided.
- Best practice: prefer impersonal imperative/infinitive phrasing for UI strings so no
  pronoun is needed at all ("Spara ändringar?" not "Vill du spara ändringar?" unless the
  source is phrased as a direct question, in which case "du" is used, never "ni").
- Sections 1-3 do not overlap: 1 is script/number rules, 2 is register/word-formation,
  3 is pronoun/address choice.

## 4. Mapping to the NEC2 electromagnetic-simulator domain

| English | Swedish (EE sense) | Notes |
|---|---|---|
| charge | laddning | electrical charge, Coulombs |
| current | ström | electrical current, Amperes |
| ground / ground plane | jord / jordplan | RF reference plane |
| wire | tråd | thin conductor in antenna model |
| gain | förstärkning | antenna directivity ratio (dB) |
| pattern | mönster | 3D directional radiation response |
| excitation | excitation / matning | electromagnetic energy input source |
| load | belastning | electrical impedance load |
| radials | radialer | horizontal ground-plane wires |
| impedance | impedans | kept as established EE loanword |
| segment / patch / tag | segment / patch / tagg | NEC2 geometry terms, unchanged sense |

Do not add a qualifier such as "elektrisk" where the source has none; program context
(an EM simulator) already disambiguates "ström" as electrical current, "laddning" as
electrical charge, etc. (see TRANSLATING.md Context-Dependent Disambiguation).

## Lexicon decisions (recorded from translation)

- **patch / Patches**: kept as the established NEC2 loanword (never translated per
  TRANSLATING.md), pluralized in Swedish grammar — "Patchar" (bare heading),
  "ytpatchar" ("surface patches", SP/SM cards), "Patchflöde" ("Patch Flow"),
  "patchströmflöde" ("patch current flow"). Do not calque to "ytlapp(ar)"; no
  established non-scoped precedent uses that form, and the one settled precedent
  ("patchytströmmar" for "--write-patch-currents") already uses "patch".
- **Power (color scale family name)**: "Effekt" — this is the dissipated-power
  transfer (n^γ), not abstract math "potens"; keep "Effekt" for "_Power"/"Power"
  everywhere the color-scale family is named.
- **Asinh / μ-law / Reinhard / Sigmoid**: mathematical/audio-engineering transfer
  function names, kept unchanged (same convention as VSWR, dBi — no Swedish
  equivalent in common use).
- **Color projection labels ("Color projection:", "Total field:", "Scale
  family:")**: GtkLabel captions for combo boxes — no mnemonic underscore, even
  when a sibling combo-box *item* elsewhere in the same dialog historically had
  one; combo-box `<item>` entries never carry GTK mnemonics.
- **Combo-box items vs menu radio items sharing the same English text**: the
  animate-dialog combo box (`glade:948-953`, "Reference phase (animated)" etc.)
  drops the "(animated)"/"(static)" qualifier's mnemonic; the *Visualization*
  menu's radio items with the same concept (`_Reference Phase`,
  `Polarization _Axis`, `Peak _Magnitude`) keep theirs — check the widget type
  (`<item>` vs `<menuitem>`/`GtkRadioMenuItem`) before deciding on a mnemonic.
- **feedpoint**: "matningspunkt" — used consistently across all matches.
- **Port**: kept as "Port" (established EE cognate, matches source).
- **dissipated power**: "avgiven effekt".
- **γ = 0.5 / 1.0 style decimal constants in prose**: kept with a period, not a
  comma, matching the file's pre-existing precedent (see `tag=%d/seg=%d: ...
  ratio of %.1f < 0.5` entries) — these are literal tokens mirroring program
  output, not general Swedish prose numbers.

## Consistency decisions (sweep)

- **tag → "tagg"** everywhere, including compounds: "Taggnummer", "Belastningstaggnummer"
  (not "…etikettnummer"), "Segmenttaggnummerfel" (not "Segmenttag…"). "etikett" =
  *label* only (eg "etikettexturatlas" for "label texture atlas"), never NEC2 tag.
- **ground → "jord"** (common gender) when the source says bare "ground"
  ("med angiven jord", "en jord finns"); **ground plane → "jordplan"** (neuter,
  "ett jordplan") only when the source says "ground plane" ("Jordplanstyp",
  "ligger i jordplanet"). Using "jordplan" for bare "ground" both over-qualifies
  and breaks adjective agreement ("*angiven jordplan").
- **Polarization → "polarisation"** (the EM quantity), never "polarisering"
  (the process): "Horisontell/Vertikal/…cirkulär polarisation",
  "_Polarisation", "Polarisationsaxel".
- **Excitation → "excitation"** everywhere, including the standalone editor tab;
  "matning"/"matningspunkt" is reserved for *feedpoint*, so do not render the
  excitation concept as "Matning".
- **File menu → "Arkiv"**: cross-references in help text name it "Arkiv->…"
  to match the "_Arkiv" menu label (not "Fil->…"). Where the English source
  itself says "Optimizer Settings" vs the menu's "Optimization Settings", the
  distinct source wording is mirrored ("Optimerarinställningar").

## Plural form

`nplurals=2; plural=(n != 1);`
