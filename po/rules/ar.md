# ar translation rules

## 1. Character set, symbology, script expectations

- Script: Arabic abjad, right-to-left (RTL); no uppercase/lowercase distinction.
- Digits: Western (LTR) digits for all technical/numeric values (`123`, `50.0`), never Arabic-Indic digits (٠١٢) — matches xnec2c's numeric/CSV output and keeps parity with NEC2 file formats.
- Punctuation: Arabic comma `،` and question mark `؟` in prose; keep Latin punctuation inside embedded LTR runs (units, mnemonics, paths).
- No tashkil (diacritics) in UI strings; modern technical Arabic omits them except where a bare consonant skeleton is genuinely ambiguous.
- Bidi embedding: technical terms, NEC2 mnemonics (`GW`, `EX`, `LD`, ...), units (`MHz`, `dBi`, `Ω`, `VSWR`), file extensions (`.nec`, `.csv`), and file paths stay LTR inside the RTL sentence; do not mirror or transliterate them.
- No word-breaking/hyphenation across lines; Kashida (ـ) justification only, never letter-spacing for emphasis — use bold instead.

## 2. Technical interface writing standards

- Register: Modern Standard Arabic (الفصحى) only; no dialectal forms (Egyptian, Gulf, Levantine, Maghrebi).
- Commands/buttons: verbal-noun (masdar) or imperative form, gender-neutral in software convention (eg "حفظ" Save, "إلغاء" Cancel, "إغلاق" Close).
- Labels/fields: nominal phrases, no verb needed (eg "التردد:" Frequency:).
- Menus/titles: noun phrases matching source capitalization pattern (Arabic has no case distinction, so this maps to word choice only).
- Sentences (dialogs, tooltips, messages): full MSA grammatical sentences, subject-verb-object or verb-subject-object, standard formal punctuation.
- GTK hotkeys: underscore precedes the mnemonic letter; choose a letter unique within its dialog and phonetically salient in the Arabic term, not a transliteration of the English mnemonic letter.
- NEC2 mnemonics, unit symbols, and file extensions are never translated (see doc/TRANSLATING.md).

## 3. Formality mapping

- No T-V distinction in Arabic; formality is carried by lexical register and sentence construction, not by pronoun choice.
- Never use second-person pronouns (أنتَ/أنتِ) or colloquial contractions in UI text.
- Achieve formality via: verbal-noun/imperative command forms, complete grammatical agreement, standard technical vocabulary over everyday synonyms, and avoidance of slang or regionalisms.
- This section is orthogonal to §1 (script mechanics) and §2 (structural conventions): formality governs word and construction *choice*, not layout or sentence type.

## 4. NEC2 domain lexicon (Arabic)

Locked terms — apply everywhere in po/ar.po, correct technical sense per doc/TRANSLATING.md disambiguation table:

| English | Arabic | Notes |
|---|---|---|
| current | تيار | electrical current (Amperes), never "حالي" (present-time sense) |
| charge | شحنة | electrical charge (Coulombs), never "فاتورة" (billing) |
| ground / ground plane | أرضي / مستوى أرضي | RF reference plane, never "تربة" (soil) |
| wire | سلك | thin conductor, never "كابل" (cable) |
| gain | كسب | antenna directivity (dB), never "ربح" (profit) |
| pattern (radiation) | نمط | radiation pattern; keep distinct from "وضع" (mode) used for render/display modes |
| mode (render/display/batch) | وضع | render/display/batch mode; never نمط (that is reserved for radiation pattern) |
| excitation | إثارة | EM energy input |
| load | حمل | impedance load, never "ثقل" (weight) |
| radials | أسلاك شعاعية | horizontal ground-plane wires |
| impedance | ممانعة | distinct from مقاومة (resistance, real R) and مفاعلة (reactance, imag X) |
| polarization | استقطاب | |
| phase | طور | |
| power (electrical) | قدرة | dissipated/radiated power; the tone-map family name "Power" stays English (see below) |
| geometry | هندسة | |
| projection (color) | إسقاط | |
| scale / scaling | تحجيم | |
| field (EM: near/total/E/H) | مجال | electromagnetic field; المجال القريب/الكلي/الكهربائي/المغناطيسي — the standard MSA physics term, applied everywhere for the EM sense |
| field (data / config) | حقل | UI/config data field only (eg "field is not registered"); never for the EM field |
| wireframe | إطار سلكي | |
| instantaneous | لحظي | |
| peak | قيمة قصوى / ذروة | قيمة قصوى for the discrete "Peak value/magnitude" UI option; ذروة for the peak/apex of a curve or frequency step in prose |
| reference (phase) | مرجعي | |

Never-translate NEC2 geometry identifiers (per doc/TRANSLATING.md — kept English/LTR):

| English | Arabic treatment | Notes |
|---|---|---|
| segment | segment (LTR) | kept English in every context (labels, field names, geometry/parser errors); القطعة only as a one-off gloss when spelling out the "seg" abbreviation |
| tag | tag (LTR) | kept English everywhere; وسم only as a one-off gloss when spelling out the abbreviation |
| patch | patch (LTR) / رقعة سطحية | keep the English NEC token "patch" in card-parser/geometry-primitive messages (eg "arbitrary patch type", "corners of quadrilateral patch"); use رقعة سطحية (pl. رقع سطحية) for the user-facing physical surface-patch object in visualization UI (Patches, Patch Flow, patch currents). Never alternate with "مصفوفة" (matrix/array), which is reserved for admittance-matrix contexts |

Tone-map / transfer-curve family names stay English as a proper-noun set: Power, dB, Asinh, μ-law, Reinhard, Sigmoid (electrical "power" still → قدرة).

VSWR, dBi, MHz, Ω, S-parameters, NEC2, and card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN) are never translated, per doc/TRANSLATING.md.
