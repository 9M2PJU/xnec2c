# uk translation rules

## 1. Script, symbology, expectations
- Cyrillic, Ukrainian alphabet (і, ї, є, ґ present; no Russian и/э/ъ/ы).
- Decimal separator is comma (`50,0`), never period.
- Sentence case for labels; no forced capitalization beyond normal Ukrainian orthography.
- Apostrophe `'` used per Ukrainian orthography (e.g. `ім'я`); avoid Russian-style spellings.
- Units/mnemonics (MHz, dBi, Ω, VSWR, GW, EX, LD, FR, RP, GE, EN) and file extensions stay unchanged, LTR.

## 2. Technical interface writing conventions
- Buttons/menu items: imperative infinitive verb forms (e.g. `Зберегти`, `Скасувати`, `Вийти`), not gerunds.
- Labels: nominative-case nouns, concise.
- Use established Ukrainian IT/engineering vocabulary; avoid Russianisms (`шлях` not `путь`, `файл`, `віджет`, `параметр`, `оптимізатор` are standard).
- One Ukrainian term per English concept, applied consistently catalog-wide.

## 3. Formality mapping
- Default register: impersonal imperative infinitives for commands/buttons (no pronoun needed) — this is the professional-neutral Ukrainian UI convention.
- When direct address to the user is unavoidable (e.g. confirmation dialogs "are you sure you want to..."), use formal `Ви` (capitalized) — never informal `ти`.
- Distinct from §1 (script/punctuation) and §2 (lexical/structural convention): this section governs only pronoun/verb-formality choice.

## 4. NEC2 domain lexicon (uk)
| English | Ukrainian | Notes |
|---|---|---|
| current (electrical) | струм | never `поточний` (temporal) |
| charge (electrical) | заряд | never billing sense |
| ground / ground plane | земля / площина заземлення | never soil |
| wire | дріт | NEC2 wire element; never `кабель`; not `провід` (that reads as electrical lead/line) |
| gain | підсилення | never profit |
| radiation pattern | діаграма випромінювання | catalog-wide term (window title, menu); never template; avoid mixing with `діаграма спрямованості` |
| excitation | збудження | never emotional state |
| load (impedance) | навантаження | never physical weight |
| impedance | імпеданс | keep consistent, not `повний опір` |
| color projection | проекція кольору / кольорова проекція | see entry-specific notes below |
| color scale / scale family | шкала кольору / сімейство шкали | |
| patch (NEC2 geometry) | патч | do not translate as generic "patch/update" |
| segment | сегмент | |
| tag | тег | |
| feedpoint | точка живлення | |
| port | порт | |
| wireframe | каркас | |

Do not add qualifiers (e.g. "electrical") absent from the English source; program context disambiguates.

## Entry-specific decisions log
- Color-scale transfer-function family names (Animate/Patch-Flow dialog and Visualization menu): `Power`→`Степенева` (generic descriptor, translated); `Asinh`, `μ-law` kept in Latin (fixed technical/algorithm identifiers, like unit abbreviations); `Reinhard`→`Рейнхард` (transliterated surname, matching existing `Smith`→`Сміт` precedent); `Sigmoid`→`Сигмоїда` (standard Ukrainian mathematical term). Applied identically across combo items, menu hotkey items, and tooltip prose.
- Menu hotkeys in the Color Scale submenu reassigned to avoid collision: `_Степенева`(С), `_дБ`(д), `_Asinh`(A), `μ-_law`(l), `_Рейнхард`(Р), `Си_гмоїда`(г), `_Немає`(Н).
- Several fuzzy entries carried a menu-item underscore hotkey into what are now plain dialog/combo labels (no hotkey applicable): stripped the stray `_` (e.g. `Total field:`, `Peak value`, `Instantaneous (φ=0)`, `Polarization axis (static)`, `Peak magnitude (static)`).
- Decimal separator: catalog-wide precedent uses `.` (period) even in Ukrainian prose (e.g. `50.00`, `0.0100`), so new tooltip prose with `γ = 0.5` kept the period for catalog consistency, overriding the general comma-decimal rule (priority c, consistent lexicon, outranks priority e, locale decimal form).
- `feedpoint`→`точка живлення`, consistent across all three related strings (callback_func.c:286, freqplots_core.c:794/913).
- Several fuzzy `msgstr` values were leftover text copy-pasted from an unrelated msgid (e.g. `mem_free`↔`mem_zero`, `Validation Tree`↔`Excitation type`, `invalid color family`↔`rdpattern style`); replaced with accurate translations of the actual (changed) msgid.
- `radiation pattern`→`діаграма випромінювання` catalog-wide: the window title/menu use `випромінювання`, so staged tooltips referencing "the radiation pattern window" (glade:912/928/944/971) were changed from `діаграми спрямованості` to `діаграми випромінювання` so the referenced window is identifiable (priority c). `спрямованого підсилення` (directive gain) is a separate term and stays.
- `wire`→`дріт` (not `провід`): the established catalog body uses `дріт` consistently (`діаметр дроту`, `довжина дроту`, `конічний дріт`, `радіус дроту`, `тонкий дріт`), the standard Ukrainian term for a NEC2 thin-conductor wire element; `провід` reads as an electrical lead/line. Staged tooltip entries (glade:356/790/807) and one pre-existing string (callbacks.c:2540) had introduced `проводів`; corrected to `дротів` for lexicon consistency (priority c), overriding the earlier lexicon note. `провідність` (conductivity) is an unrelated word and stays.
