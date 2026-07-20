# uk translation rules

## 1. Script, symbology, expectations
- Cyrillic, Ukrainian alphabet (і, ї, є, ґ present; no Russian и/э/ъ/ы).
- Decimal separator is comma (`50,0`), never period.
- Sentence case for labels; no forced capitalization beyond normal Ukrainian orthography.
- Apostrophe `'` used per Ukrainian orthography (e.g. `ім'я`); avoid Russian-style spellings.
- Physical-unit abbreviations localize to the standard Ukrainian (ДСТУ) Cyrillic forms: `Гц`, `кГц`, `МГц`, `дБ`; apply catalog-wide (labels, graph titles, log/status messages).
- Kept unchanged, LTR: `dBi` (compound RF gain unit, consistently Latin across the catalog), `VSWR`, `Ω`, NEC2 mnemonics (GW, EX, LD, FR, RP, GE, EN), S-parameter/figure-of-merit tokens (`S11`, `F/B`, `G/Ta`, `Z0`), and file extensions.

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
- Animate-dialog chroma projection dialog (color source picker, resources/animate.glade, and its src/config_hooks.c/chroma_source.c strings): translated as a batch of untranslated + fuzzy entries. Category headers `Animated`/`Static` (radio-group labels above the projection list) rendered as adverbs `Анімовано`/`Статично` to match the parallel `Animated: …`/`Static: …` lead-in already used inside each entry's tooltip (priority c, consistent lexicon between header and body).
- `Polarity` (projection name, distinct from `_Polarization` elsewhere in the catalog) → `Полярність`, not `Поляризація`: a prior fuzzy entry had copy-pasted the unrelated `_Polarization` translation onto this changed msgid; `полярність` (sign +/−) and `поляризація` (antenna polarization) are different concepts and both terms now coexist correctly at their own msgids.
- Several fuzzy entries in this dialog carried leftover msgstr text copy-pasted from an unrelated (often only superficially similar) msgid: `Current + Charge`↔`Джерело струму` (current source), `Far-field Contribution`↔`Анімація ближнього поля` (near field animation), `Comet`↔`Геометрія` (geometry), `unhandled hue/brightness encoding`↔`invalid color projection`'s string, `invalid palette kind`↔`invalid color projection`'s string, `create_animate_dialog` load-failure↔`render_settings_init`'s string. Replaced each with an accurate translation of its own (changed) msgid; see priority c.
- Multi-paragraph tooltip msgids in this dialog use a blank line (`\n\n`) between sentences; several fuzzy entries had collapsed this to a single `\n` (or added a trailing instruction sentence, e.g. "Adjust k in the animate dialog," absent from the new shorter msgid). Reformatted to match the msgid's paragraph structure exactly rather than preserving the old prose shape.
- `48.16 dB` (μ-law tooltip): kept the period per the existing catalog-wide decimal-point precedent (see `γ = 0.5` note above), not comma.
- Unit Cyrillicization consistency sweep: the catalog localizes `dB`→`дБ` and `MHz`→`МГц` in the overwhelming majority of entries (staged `dB` combos/labels, `(МГц)` frequency labels, `Гц` animation-rate). Leftover Latin `dB` (graph titles `G/Ta (dB)`, `F/B Ratio dB`, `S11 dB`) and Latin `MHz` (log strings `xnec2c.c:708`/`:1043`, mathlib tooltip widget reference) were converged to Cyrillic for priority-c consistency; `%.6f`/`%g` format specifiers and the surrounding numeric fields were left intact. `dBi` deliberately stays Latin at every site (`Raw Gain dBi`, `Net gain dBi`, average-gain log), matching its consistent catalog treatment and RF convention.
