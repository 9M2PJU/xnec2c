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

## Lexicon decisions (round 2 — chroma/animate subsystem)

- **hue** (chroma color-projection axis, distinct from "color" in general):
  "färgton" — used for hue-driven tooltip prose ("Färgtonen visar...",
  "Divergerande färgton visar..."). Distinct from **tone** (the tone-transfer
  *family* concept, eg `color_tone.c`, "color tone"): that maps to
  "skalfamilj", matching the established UI label "Scale family:" =
  "Skalfamilj:" — never "färgfamilj" (superseded; was an inconsistent
  pre-existing translation for the same concept, corrected for
  catalog-wide consistency).
- **Polarity** (sign-of-quantity color axis, "Polarity"/"Polaritet"): kept
  distinct from **Polarization** ("Polarisation") — a false-cognate error
  existed where "Polarity" was mistranslated as "_Polarisation"; these are
  different EM/UI concepts and must never share a translation.
- **Instantaneous** (bare projection-mode label): "Momentan" with no
  "(φ=0)" qualifier when the English source itself has none; the
  qualifier is added only where the source spells it out, eg "instantaneous
  (φ=0)" in the near-field vector chooser → "momentana (φ=0)".
- **palette kind** (internal enum distinct from "color projection" and
  from "scale family"/tone): "palettyp". Its named fallback value "ramp"
  renders as "rampskala" (parallel to "Rainbow ramp" → "Regnbågsskala",
  which also drops bare "ramp" for "-skala").
- **wave crest** (animation/comet-overlay tooltips): "vågtopp".
- **Overlays:** (animate-dialog section label) → "Overlag:"; **overlay**
  (the derived-quantity concept in disabled-widget tooltips, eg
  config_hooks.c) → "overlag" (neuter), lowercase in running prose.
- **γ = 0.5 / 48.16 dB style decimal constants in prose**: kept with a
  period, not a comma — same precedent as the existing γ = 0.5 rule;
  applies to any literal numeric constant naming a mathematical/standards
  quantity in these tone-transfer tooltips (μ-law's "48.16 dB" is the
  fixed μ = 255 companding constant, not a general Swedish prose number).
- **Multi-paragraph tooltip msgids using a blank `\n\n` line between
  sentences**: the Swedish msgstr must reproduce the same blank-line
  paragraph break at the same position; several fuzzy entries had a
  stale msgstr collapsing two msgid paragraphs onto adjacent lines with a
  single `\n` — always mirror the msgid's exact `\n` vs `\n\n` structure.
- **Stale "Mirrors the X button/menu in the main window." trailing
  sentences**: several fuzzy entries carried an extra second sentence
  referencing a sibling widget that no longer appears in the current
  (shortened) English msgid; when the msgid no longer has that clause,
  drop it from msgstr rather than preserving it.

## Lexicon decisions (round 3 — animate/settings staged review)

- **radiation pattern → "strålningsdiagram"**: the established, catalog-wide EE
  term (all menu/title/label entries: "_Strålningsdiagram", "Strålningsdiagram
  (RP-kort)", "Strålningsdiagramparametrar", etc.) and standard Swedish antenna
  usage. This overrides the generic `pattern → mönster` mapping in section 4 for
  this compound; "mönster" stays only for a non-antenna repeating pattern/template.
  Corrected staged strays "strålningsmönster(data)" → "strålningsdiagram(data)"
  (near E/H/Poynting field tooltips and the far-field-contribution error string).
- **hue → "färgton"** applies to labels too: "Phase Hue" → "Fasfärgton" (corrected
  from "Fasnyans"; "nyans" = nuance/shade, not the codified hue term). Exception:
  the fixed compound "hue wheel" stays the idiomatic "färghjul" (color wheel).
- **"Visual only" → "Endast visuellt"**: adverbial word order; "Visuell endast" is
  an unnatural English calque.
- **bare "ground" → "jord/jorden"** reaffirmed: "extends below ground" =
  "sträcker sig under jorden", never "under jordplanet" (that reads the msgid as
  "ground plane", which it is not). "ligger i jordplanet" is correct only where
  the msgid says "lies in ground plane".

## Plural form

`nplurals=2; plural=(n != 1);`
