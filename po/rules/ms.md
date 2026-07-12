# Malay (ms) Translation Rules for xnec2c

## Script & Orthography
- Standard Rumi (Latin) script, no diacritics required for Malay.
- LTR, standard Latin punctuation.
- No noun capitalization (unlike German); sentence-case labels — only first word/proper nouns capitalized, not every word of an English title-case string.
- Decimal separator: period `.` (Malaysian convention matches English, NOT comma).
- Numbers, file paths, NEC2 mnemonics, and units remain LTR/unchanged inside Malay text.

## Formality Register
- No T-V pronoun distinction like European languages. Prefer imperative verb forms for menu items/buttons/labels (e.g. "Simpan" not "Anda simpan").
- When a direct address to the user is unavoidable (dialogs, confirmations, questions), use "anda" (neutral formal you). Never use "kamu" or "awak" (too informal/colloquial for professional software).
- Register: formal-neutral, DBP (Dewan Bahasa dan Pustaka)-standard technical Malay. No bazaar/colloquial Malay (Melayu pasar).

## Plural Forms
- `nplurals=1; plural=0;` — Malay does not inflect nouns for plural.

## Terminology Lock (do not translate)
- NEC2 card mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN, GD, GN, GC, GX, GR, GS, GM, GF, CP, CM, NE, NH, NT, TL, SP, SC, SM, SY, XQ, EK, KH
- Units/abbreviations: VSWR, dBi, MHz, Ω, dB, S-parameters (S11, S1P, S2P), Zo, Z
- File extensions: .nec, .csv, .s1p, .s2p, .png, .gplot
- Software/program name: xnec2c, NEC2, gnuplot, OpenGL, Cairo, MKL, OpenBLAS, OpenMP

## Domain Glossary (electrical sense — disambiguation)
| English | Malay | Notes |
|---|---|---|
| current (electrical) | arus | electrical current, not "semasa" (temporal) |
| charge (electrical) | cas | electrical charge, not "caj" (billing/fee) |
| ground / ground plane | bumi / satah bumi | electrical reference, not "tanah" (soil) |
| wire | wayar | thin conductor |
| gain | gandaan | antenna directivity ratio |
| pattern (radiation) | corak (sinaran) | 3D directional response |
| excitation | pengujaan | EM energy input |
| load (impedance) | beban | electrical impedance, not physical weight |
| radials | jejari | ground-plane radial wires |
| impedance | galangan | |
| reactance | reaktans | eg "Reactance Ohm" → "Reaktans Ohm" |
| admittance | admitans | eg "Admittance Matrix" → "Matriks Admitans" |
| noise (temperature) | hingar | "Noise Temperature" → "Suhu Hingar"; noise in the signal/thermal sense |
| efficiency | kecekapan | |
| segment | segmen | NEC2 geometry term, keep as-is |
| patch | tampalan | surface patch geometry |
| tag | tag | NEC2 identifier, keep as English loanword |
| frequency | frekuensi | |
| voltage | voltan | |
| resistance | rintangan | |
| inductance | induktans | |
| capacitance | kapasitans | |
| conductivity | kekonduksian | |
| near field | medan dekat | |
| far field / radiation pattern | corak sinaran | |
| structure | struktur | |
| geometry | geometri | |
| optimizer | pengoptimum | |
| threshold | ambang | |
| overlap | pertindihan | |
| total (gain/field) | jumlah | quantifier precedes noun: "Jumlah Gandaan", "Jumlah Medan" — never "Gandaan Jumlah" / "Medan Jumlah" |
| major axis | paksi utama | |
| minor axis | paksi kecil | eg "Minor/Major Axis" → "Paksi Kecil/Utama"; never loanwords "Minor/Major" |

## Disambiguation Policy
- "loading": distinguish the gerund (reading/parsing a file or card → memuatkan/pemuatkan) from the electrical LD load (→ beban). "Loading Command (LD Card)" → "Arahan Beban"; "loading data card error" (error while parsing) → "kad data pemuatan".
- Use the correct electrical-domain sense per the glossary above.
- Do NOT add qualifiers ("elektrik") absent from the English source. The EM-simulation context disambiguates for domain-expert users (e.g. "View Currents" → "Lihat Arus", not "Lihat Arus Elektrik").

## Format Specifiers
- Preserve %s, %d, %f, %c, %% in identical order (Malay SVO word order rarely requires reordering).

## GTK Hotkeys (`_` mnemonics)
- Choose mnemonic letters that are common, typable Malay/Latin keyboard letters and do not collide within the same dialog/menu. Shift to an alternate letter within the translated word when a collision would occur, matching Malay word conventions.

## UI Conventions
- Menu items/buttons: concise imperative verbs (Simpan=Save, Buka=Open, Keluar=Quit/Exit, Padam=Delete, Batal=Cancel, Tutup=Close).
- Axis names X/Y/Z remain uppercase.
- Dialog confirmations use "anda" (formal you), e.g. "Adakah anda pasti...?"
