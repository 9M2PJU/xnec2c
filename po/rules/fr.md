# fr translation rules

## 1. Character set, symbology, expectations

- Latin script with French diacritics: é è à ç ê î ô û ù œ ï ü â.
- Typographic apostrophe `'` (U+2019) preferred in careful prose, but ASCII `'` is
  acceptable and already used throughout this catalog for consistency; do not mix.
- Guillemets `« »` (with a narrow/regular space inside) are the native quotation
  marks, but existing entries use plain `'...'` for widget names; keep that
  convention for consistency rather than introducing guillemets mid-catalog.
- Decimal separator is the comma (`50,0`), but any hard-coded numeric literal
  inside a msgid (eg `%g`, `10^(-R/20)`) is data, not prose, and stays untouched;
  only prose-visible example numbers written out in words would use `,`.
- Non-breaking space conventionally precedes `:` `;` `!` `?` in print French;
  this is a UI catalog where source strings already omit it before `:` — match
  the existing catalog convention (plain space before `:`, no nbsp) for
  consistency with prior entries (eg "Scale family:" -> "Famille d'échelle :"
  already uses a space before the colon in prior entries such as line 351 area).
  Use a normal space before `:` to match existing msgstr practice in this file.

## 2. Technical interface writing standards

- Minimal capitalization: only the first word and proper nouns/acronyms
  (NEC2, VSWR, GTK, NEC card mnemonics) are capitalized; menu/button labels
  are sentence case, not title case, even where the English source is
  title case (eg "Color Scale" -> "Échelle de couleur", not "Échelle De Couleur").
- Infinitive or imperative mood for commands/buttons ("Réinitialiser", not
  "Réinitialise" or "Vous réinitialisez"); noun phrases for labels and titles.
- Keep NEC2 mnemonics, unit symbols (MHz, dBi, Ω, VSWR, dB), and file
  extensions unchanged, per doc/TRANSLATING.md.
- GTK accelerator `_` only where the msgid carries one; never invent a
  `_` accelerator on a label whose source has none (eg "Color Quantization"
  -> "Quantification de couleur", not "de _couleur").

## 3. Formality mapping

- Formal register: `vous` implied via infinitive/impersonal phrasing in all
  UI strings (buttons, dialogs, tooltips, error messages); never `tu`/`ton`.
  Existing entries already follow this (eg "Sélectionner...", "Miroir du
  paramètre...").
- Error/log messages (pr_debug, pr_err, developer-facing) stay neutral/
  impersonal, third person, no direct address at all.

## 4. NEC2 domain lexicon (established in this catalog — reuse exactly)

| English | French | Notes |
|---|---|---|
| current (electrical) | courant | never "actuel" |
| charge | charge | electrical charge |
| ground (soil/reference) | sol | "Ground Plane" -> "plan de masse" |
| ground plane | plan de masse | RF sense |
| wire | fil | thin conductor |
| radial(s) | radial / radiale / radiaux | adjective agrees with noun |
| gain | gain | antenna directivity, not profit |
| pattern (radiation) | diagramme (de rayonnement) | |
| excitation | excitation | |
| load (impedance) | charge / impédance de charge | context-dependent, matches source qualification |
| feedpoint | point d'alimentation | |
| viewer (observation direction) | observateur | "Viewer Gain" -> "Gain observateur"; gain in the viewing/observation direction |
| viewer (3D display widget/panel) | visualiseur | masculine; UI label "Viewer" -> "Visualiseur"; agrees masc (visualiseur réinitialisé) |
| impedance | impédance | |
| segment / patch / tag | segment / patch / tag | NEC2 geometry terms, unchanged |
| color projection | projection de couleur | new render-settings feature |
| color family / scale family | famille de couleur / famille d'échelle | |
| animate / animation | animer / animation | |
| widget | widget | technical/dev-facing, unchanged |
| managed allocator / mem-report | allocateur géré / rapport mémoire | dev-facing log strings |
| threshold / knee | genou (softening knee), seuil (generic threshold) | signal-processing sense |

## 5. Notes

- Dev-facing / pr_debug-style log strings (mem_track, config_widget,
  themes, validation_dump, prerender) still get full French translation in
  this catalog (LOW priority per doc/TRANSLATING.md does not mean "skip");
  keep them literal, neutral, and terminologically consistent with the table
  above.
- Do not add qualifiers absent from the English source (eg do not turn
  "View Currents" into "Voir les courants électriques"); the NEC2/RF context
  disambiguates "courant" without help.
