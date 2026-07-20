# pt translation rules

## 1. Native character set, symbology, and expectations

- Script: Latin, Portuguese orthography (post-1990 Orthographic Agreement, as used in Brazil).
- Diacritics: á, â, ã, à, ç, é, ê, í, ó, ô, õ, ú; nasal vowels (ã, õ) and cedilla (ç) are mandatory, never dropped.
- Decimal symbol: comma in general Portuguese usage (`50,0`), but this catalog's existing entries keep the source's `%f`/`%d` numeral formatting as-is; do not alter numerals inside format specifiers or literal English unit tokens (`MHz`, `dBi`, `Ω`, `VSWR`).
- Quotation: Portuguese normally uses angle quotes «...», but this codebase's UI/CLI strings quote literal argument names with straight `'...'`; preserve straight quotes as they are structural (argument delimiters), not prose quotation.
- Ellipsis and punctuation follow Portuguese convention (space before `:` is NOT used, unlike French; keep `:` attached as in source).

## 2. Writing standards for a technical program interface in this language

- Brazilian Portuguese (pt-BR) technical/software register: infinitive or imperative verb forms for commands and buttons (eg "Selecionar", "Ativar"), noun phrases for labels and titles.
- Sentence case, not title case, for full sentences and tooltips; UI labels/menu items may keep each significant word capitalized only where the source does (mirror source capitalization pattern rather than imposing German-style noun capitalization).
- Compound technical nouns are written as separate words or hyphenated per Brazilian norms, not run together (eg "ganho líquido", not "ganholíquido").
- Software brand/product names (xnec2c, NEC2) and card mnemonics (GW, EX, LD, FR, RP, GE, EN) are never translated or inflected.
- File paths, filenames, and CLI flags (`--batch`, `-j`) remain verbatim; only the surrounding descriptive text is translated.

## 3. Formality and informality

- Register: formal, addressing the user with implicit "você" (per doc/TRANSLATING.md guidance), but Brazilian technical software conventionally omits the pronoun itself and uses imperative/infinitive verb forms instead (eg "Selecione..." / "Selecionar..."), which is the formal-neutral default and matches the existing catalog's tone.
- Avoid the second-person informal "tu" conjugations entirely (not used in Brazilian technical writing).
- Avoid overly bureaucratic/archaic constructions (eg "Vossa Senhoria"); keep register professional but direct, matching engineering-tool tone.
- These three sections do not overlap: §1 is orthography/symbols, §2 is sentence/word construction and capitalization, §3 is pronoun/verb-form register choice.

## 4. Mapping to the NEC2 electromagnetic simulation domain

- current → corrente (electrical, Amperes) — never "atual" (temporal "current").
- charge → carga (electrical, Coulombs) — never "cobrança" (billing).
- ground / ground plane → terra / plano de terra (RF reference) — never "solo" (dirt). This holds for every ground sub-sense: ground conductivity → "condutividade da terra", ground effects → "efeitos da terra", Sommerfeld ground → "terra de Sommerfeld", ground model/type → "modelo/tipo de terra". One term ("terra") for the whole "ground" meaning; "solo" never appears.
- wire → fio (thin conductor) — never "cabo" (cable/cord).
- gain → ganho (antenna directivity, dB) — never "lucro" (profit).
- pattern (radiation/field pattern) → diagrama (eg "diagrama de radiação", "diagrama de campo", "diagrama no plano XZ") — never "modelo" (template/design). Use "diagrama" consistently for pattern; reserve "padrão" for "default"/"standard" so the two meanings never collide on one word.
- viewer (structure viewer / observer direction) → visualizador consistently (eg "ganho do visualizador", "direção do visualizador") — never mix with "observador".
- excitation → excitação (EM energy input) — never emotional sense.
- load → carga (impedance, note: shares Portuguese word with "charge"; disambiguated by context — "carga" alone in an LD-card/impedance context, never "peso"/"carregamento" for weight/loading UI action).
- patch (NEC surface patch) → patch (kept as the NEC2 geometry term, per doc/TRANSLATING.md "segment, patch, tag" never-translate list); do not confuse with GTK "patches" tab which also refers to this same geometry term.
- Never translate: NEC2, card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), VSWR, dBi, S-parameters, Z, file extensions (.nec, .csv, .s1p, .s2p, .png), unit symbols (MHz, dBi, Ω, degrees).
- Disambiguation is selected by technical sense only; do not append explanatory qualifiers (eg "elétrica") absent from the English source (per frame.md Disambiguation and doc/TRANSLATING.md policy). Eg "View Currents" → "Ver Correntes", not "Ver Correntes Elétricas".
- exit (process termination) → "encerramento"/"encerrado" (matching "child process ... exited" → "processo filho ... encerrado"). Do NOT render "default exit" as "saída padrão": in software pt "saída padrão" is the fixed term for stdout, a false collision. Use "encerramento padrão" for the default/fallback exit branch. Reserve "saída" for stdout/CLI output senses.

## Casing (axis names)

- Coordinate axis letters X/Y/Z are uppercase in pt even when the English source lowercases them (eg source "step size limited at z=" → "...limitado em Z="). Apply uniformly across all sibling strings so the same axis never appears both "z" and "Z".

## Hotkey policy

- GTK mnemonics use a leading underscore before the accelerator letter (eg `_Potência`).
- When translating produces a duplicate mnemonic within the same menu/dialog, shift the underscore to a different, still-mnemonic and typable letter for pt-BR (common keyboard, no dead-key-only letters as sole accelerator).
