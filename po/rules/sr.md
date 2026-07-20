# sr translation rules

## Script and locale
- Script: Cyrillic (ћирилица) exclusively; this catalog's existing convention uses Cyrillic throughout (е.g. "Заједничка", "Фаза"). Do not switch to Latinica.
- Latin-script technical tokens stay embedded unchanged inside Cyrillic sentences: NEC2 card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), units (MHz, dBi, Ω), VSWR, S-parameters, file extensions (.nec, .csv, .s1p, .s2p), internal identifiers/function names (mem_free, config_widget_*, eventfd, inotify).
- No case-shift concept beyond standard sentence/proper-noun capitalization; do not add German-style noun capitalization.
- Numbers/format specifiers (%s, %d, %f, %c, %%) unchanged and in source order.

## Register
- No explicit "Ви" pronoun in UI strings; polite address is carried by 2nd-person-plural imperative verb forms alone (matches existing corpus, e.g. "Изаберите..."). Tooltips/status text use descriptive indicative mood, third person or impersonal, not "Ви".
- Imperative for actions/buttons; indicative/impersonal for status, error, and tooltip text.

## Domain lexicon (reuse everywhere; do not vary)
| English | Serbian (Cyrillic) | Notes |
|---|---|---|
| current (electrical) | струја | never "тренутни" (temporal) |
| charge (electrical) | наелектрисање | |
| excitation | побуда | |
| gain (antenna) | добитак | standard SR antenna term (Đorđević ETF; Wikipedia "Добитак антене"); never "појачање" (that is amplifier amplification) nor "профит" |
| viewer (direction/observer) | посматрач | gain-in-viewer-direction sense; matches existing "посматрања"; not "прегледач"/"приказивач" |
| impedance | импеданса | |
| pattern (radiation) | дијаграм (зрачења) | |
| wire | жица | |
| segment | сегмент | |
| patch (surface) | патч / закрпа | "патч" as noun-tag, "закрпа/закрпе" in descriptive phrases (existing: "површинске закрпе") |
| theme | тема | |
| widget | виџет | |
| scale | скала / скалирати | |
| animate/animation | анимирати / анимација | |
| frequency loop | фреквенцијска петља | |
| validation | верификација | |
| optimizer | оптимизатор | |
| port (feedpoint) | порт | |
| brightness | осветљеност | |
| knee (compressor curve) | колено | |
| gamma exponent | гама (експонент) | |
| softening | омекшавање | |
| contrast steepness | нагиб контраста | |
| dynamic range | динамички опсег | |
| compression | компресија | |
| hue | нијанса | |
| polarity | поларитет | |
| magnitude | амплитуда | consistent across peak/instantaneous magnitude entries |
| color projection | пројекција боје | |
| geometry | геометрија | |
| feedpoint | тачка напајања | |
| overlay (graphic annotation layer) | преклоп | masc. noun; matches existing "cairo преклоп сенчара"; plural "преклопи" |
| comet (moving-crest marker) | комета | |
| encoding (hue/brightness) | кодирање | distinct from "пројекција" (projection); do not collapse hue/brightness encodings to one shared msgstr |
| radius | полупречник | native technical term; never the loanword "радијус" (was inconsistent in geometry.c/geom_edit error strings) |
| radials (ground wires) | радијали | horizontal ground-plane wires; distinct from "полупречник" |
| kernel (integral-equation / thin-wire) | језгро | mathematical kernel term; never the transliteration "Кернел" (unify UI EK-card labels with geometry.c messages) |
| cliff (two-medium ground boundary) | литица | Linear/Circular Cliff ground types; matches "Други медијум (литица)"; never "прелом" |
| token (expression parser) | токен | not left as Latin "token" |
| kernel-value coordinate readout z | z (lowercase) | keep the source-lowercase "z=" in "step size limited at z=" messages; do not uppercase to axis-label "Z" |

## Disambiguation
- Choose the electrical/domain sense directly (струја=current, наелектрисање=charge, добитак=gain, импеданса=load) without adding a qualifying adjective ("електрична") absent from the English source, per frame disambiguation rule.

## Script hygiene
- The catalog historically leaked Latin-script (latinica) Serbian words into otherwise-Cyrillic msgstrs (eg `za`, `kada`, `koji`, `jednom`, `ali`, `možete`, `nakon`, `Generisan atlas tekstura oznaka`) and homoglyph mixes (`objекат`, `nepознати`, `иmе`, `даљe`). All Serbian text is Cyrillic; only genuine Latin technical tokens stay Latin. Sweep with two regexes: `[čćžšđČĆŽŠĐ]` (Latin-only diacritics never valid here) and `[A-Za-z][А-Яа-я]|[А-Яа-я][A-Za-z]` (homoglyph adjacency); both must return zero non-token hits.
- Translate plain English words that are not identifiers/units (eg `legacy` → `наслеђено`); keep API/identifier tokens (inotify, eventfd, FBO, mathlib, MKL, threading).

## Format specifiers
- Preserve %s/%d/%c/%f/%% in the same left-to-right order as the msgid; Serbian free word order lets sentence structure adapt around fixed specifier positions.
