# es translation rules

## 1. Native character set, symbology, expectations

- Latin script with Spanish diacritics: á é í ó ú ü ñ.
- Inverted opening punctuation for questions/exclamations: ¿...? ¡...!
- Straight quotes `'…'` / `"…"` for UI strings (matches existing catalog usage); literary «…» not used in this interface.
- Decimal point `.` retained as in source (neutral Latin American technical convention; NEC2 numeric values are not locale-reformatted).
- Unit symbols, NEC2 mnemonics, and file extensions are never transliterated: MHz, dBi, Ω, VSWR, dB, GW, GA, GH, EX, LD, FR, RP, GE, EN, SP, SM, NE, NH, .nec, .csv, .s1p, .s2p, .png.

## 2. Technical program interface writing standards

- Infinitive verb form for menu items and commands (e.g. "Guardar", "Cancelar"), not conjugated imperative, matching established Spanish software convention.
- Minimal capitalization: only the first word and proper nouns are capitalized in labels/titles (sentence case), never title case per word.
- Concise labels; omit articles where the English source omits them.
- Domain-standard abbreviations (VSWR, dBi, S-parameters, Z) stay in Latin/English form; they are recognized as-is by Spanish-speaking RF engineers.

## 3. Formality and informality mapping

- Use formal "usted" register throughout; never "tú"/"vos" conjugations.
- Dialogs addressing the user use usted verb forms: "¿Está seguro...?", not "¿Estás seguro...?".
- Prefer neutral Latin American vocabulary over Iberian-specific terms when both exist (e.g. avoid regionalisms), for broadest reach.
- This section governs register/pronoun choice only; it does not overlap with orthography (§1) or capitalization/word-form conventions (§2).

## 4. NEC2 electromagnetic domain mapping

Never translate (identifiers/units, left exactly as source):
- NEC2, card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN, SP, SM, NE, NH), file extensions (.nec, .csv, .s1p, .s2p, .png), unit/technical symbols (MHz, dBi, Ω, VSWR, dB, S-parameters, Z).

Translate as established Spanish EE nouns (already used consistently in this catalog; keep this mapping):
- segment → segmento, patch → parche, tag → etiqueta (geometry nouns, not identifiers).
- current → corriente (electrical sense; never "actual/presente").
- charge → carga (electrical sense; never "cargo/costo").
- ground / ground plane → tierra / plano de tierra (RF reference; never "suelo" in the casual sense).
- wire → hilo/conductor (thin antenna conductor; never "cable").
- gain → ganancia (directivity ratio; never "beneficio/ganancia económica").
- pattern → patrón / diagrama (radiation pattern; never "plantilla/diseño").
- excitation → excitación (EM energy input; never emotional sense).
- load → carga/impedancia de carga depending on context (never "peso").
- radials → radiales (ground-plane wires; never "relativo al radio" sense).

Color/animation subsystem (keep one term per meaning across the catalog):
- hue → tono; brightness → brillo; projection → proyección.
- scale family → familia de escala; the `color_tone` enum error ("invalid color tone") maps to "familia de escala" so runtime messages match the user-facing UI label, not literal "tono".

Disambiguation policy: choose the correct technical sense implicitly from the antenna-simulator context; do not append clarifying qualifiers absent from the English source (e.g. "Ver Corrientes", not "Ver Corrientes Eléctricas").

## 5. Hotkey policy

GTK `_` mnemonics must stay unique per dialog/menu; when the natural Spanish first letter collides with an existing hotkey in the same UI, shift the underscore to another letter that is genuinely typable and expected by Spanish keyboard users (e.g. `_Cancelar` and `C_errar`, not two `_C`).
