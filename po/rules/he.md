# he translation rules

## 1. Native character set, symbology, expectations

- Hebrew alphabet (אלף-בית עברי), written right-to-left (RTL); no uppercase/lowercase distinction — casing rules from Latin-script guidance do not apply to Hebrew letters themselves.
- Niqqud (vowel points) omitted in modern technical/UI text; standard unvocalized Hebrew is used throughout.
- Digits are Western Arabic numerals (0-9) and render left-to-right even inside an RTL sentence, per the Unicode Bidi Algorithm (eg "123 MHz" stays "123 MHz", not mirrored).
- Latin acronyms, NEC2 mnemonics, unit symbols, and file extensions are written in Latin script embedded in the RTL sentence and stay LTR (eg `VSWR`, `dBi`, `Ω`, `GW`, `.nec`).
- Hebrew punctuation marks — geresh (׳) for abbreviation/single letter, gershayim (״) for acronyms — exist but are not required for retained-Latin acronyms; do not add them to preserved English tokens.
- Maqaf (־) is the Hebrew hyphen, used to bind compound technical terms when natural (eg מצב-אצווה).

## 2. Technical program interface writing standards

- Menu items, command labels, and buttons use nominal/infinitive command forms (eg "שמירה" / "לשמור") rather than blunt second-person imperative, matching established Hebrew software localization convention (Office, GNOME/GTK Hebrew locales).
- Dialogs and questions prefer impersonal phrasing over direct second-person address where natural, since Hebrew's second person is gender-marked (אתה/את) and impersonal phrasing avoids forcing a gender choice (eg "האם לצאת מ-xnec2c?" rather than "האם אתה בטוח שברצונך לצאת?").
- Keep established Hebrew engineering vocabulary already present in this catalog; do not introduce synonyms for terms already translated elsewhere (see §4 glossary).
- Loanwords/transliterations are acceptable only where no standard Hebrew engineering term exists or where the transliteration is the term of art in the Israeli amateur-radio/EE community (eg רדיאלים for radials).
- NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), unit symbols (MHz, dBi, Ω, degrees), file extensions (.nec, .csv, .s1p, .s2p, .png), and other Never-Translate terms from doc/TRANSLATING.md stay in Latin script, untranslated.
- Follow doc/TRANSLATING.md's context-dependent disambiguation policy: select the correct electrical/technical sense, but do not append a qualifier (eg "חשמלי") absent from the English source.

## 3. Formality / address register

- Hebrew has no separate formal/informal second-person pronoun (unlike Sie/du, vous/tu); "formality" here is achieved through register and address strategy, not pronoun choice:
  - Prefer impersonal/nominal phrasing (§2) over direct second-person address to sidestep gender marking and read as professional.
  - Where second-person address is unavoidable, default to the masculine form (אתה/-ך), the standard generic form in existing Hebrew technical software; this is a register/genderdefault choice, distinct from the orthographic rules in §1 and the phrasing-structure rules in §2.
  - Maintain a professional, neutral tone throughout; avoid slang, colloquial contractions, or casual particles.
- Confirmation dialogs use the impersonal infinitive question, never the gender-forcing "אתה בטוח שברצונך" construction (§2): "Are you sure you wish to quit xnec2c?" → "האם לצאת מ-xnec2c?"; "…end the calculation?" → "האם לסיים את החישוב?"; "…close this window?" → "האם לסגור חלון זה?".

## 4. NEC2 electromagnetic-simulator domain glossary (Hebrew)

Established terms already used in this catalog — reuse them consistently, do not introduce alternatives:

| English | Hebrew | Notes |
|---|---|---|
| current (electrical) | זרם | not "עדכני/נוכחי" (temporal) |
| charge (electrical) | מטען | not billing sense |
| gain (antenna) | הגבר | not רווח (profit) |
| ground / ground plane | הארקה / מישור הארקה | not אדמה (soil); antenna/electrical ground reference and the GN/GD/GE ground cards, Ground Type options (Perfect/Sommerfeld/Radial), Ground Conductivity, Ground Plane type |
| earth / ground (physical medium) | קרקע | physical earth/terrain medium: "earth" thermal/noise models, "ground wave" propagation (גל קרקעי), "extends below ground" (מתחת לקרקע); distinct from electrical הארקה — do not merge |
| gain (antenna) — reaffirm | הגבר | never רווח (profit), even in compounds like "Gain Style" → סגנון הגבר |
| polarity | קוטביות | distinct from polarization (קיטוב); "Instantaneous Polarity" → קוטביות רגעית |
| impedance | עכבה | keep Z symbol where source uses it |
| excitation | עירור | |
| load (electrical) | עומס | not משקל (weight) |
| wire | תיל | not כבל (cable), not חוט (thread); unify all "wire" senses (Wire Diameter, Thin Wire Kernel, wire radius, radial wires, structure wire segments) to תיל |
| radials | רדיאלים | established ham-radio transliteration; noun "Radials" (eg "Circular Cliff + Radials") → רדיאלים, distinct from the plural adjective "radial wires" → תילים רדיאליים |
| segment | מקטע | NEC2 geometry term |
| patch (surface patch) | טלאי | |
| pattern (radiation pattern) | תבנית (קרינה) | not תבנית as "template" |
| magnitude | עוצמה / גודל | עוצמה for signal strength, גודל for scalar magnitude — match existing usage per string |
| phase | פאזה | |
| polarization | קיטוב | |
| geometry | גאומטריה | |
| frequency | תדר | |
| structure | מבנה | |
| model | מודל | not דגם; unify every "model" sense (NEC model, noise-temperature models) |
| plot / graph | גרף (רבים: גרפים) | "Frequency (Data) Plots" → גרפי (נתוני) תדר; not תרשים |
| linear | לינארי | standard כתיב מלא — the tzere on נ is not written with yod, so not ליניארי; applies to "linear algebra/power/voltage/wave", גל פוגע לינארי, אלגברה לינארית |
| scale | קנה מידה | |
| scale family / color tone (transfer curve) | משפחת קנה מידה | Power/Log/Asinh/μ-law/Reinhard/Sigmoid/None; "color tone" in dev messages is this same family, not משפחת צבע |
| standing wave / traveling wave | גל עומד / גל נע | short-label pairing "Standing/Traveling" → עומד/נע |
| null (current) / peak (current) | אפס / שיא | "Nodes/antinodes" UI label and "nulls...peaks" tooltip both map to this pair for consistency |
| comet (visual overlay) | שביט | animated bright-head overlay riding the wave crest; not גאומטריה |
| far-field | שדה רחוק | contrasts near-field שדה קרוב; "Far-field Contribution" → תרומת שדה רחוק |

Never-translate list (per doc/TRANSLATING.md): NEC2 card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), VSWR, dBi, S-parameters, Z, .nec/.csv/.s1p/.s2p/.png, MHz, Ω, degrees.

## 5. Casing and hotkeys

- No case distinction in Hebrew letters; GTK `_` hotkey mnemonics precede a Hebrew letter of the key word (eg `_רגעי`, `_גודל`, `_קיטוב`).
- When two hotkeys in the same menu/dialog would collide on the same letter, shift the underscore to a different, naturally salient letter in one of the two labels rather than duplicating.

## 6. RTL layout

- Numbers, NEC2 mnemonics, unit symbols, and file paths remain LTR inside RTL sentences (Unicode Bidi Algorithm handles embedding automatically; do not add manual direction overrides).
- Preserve source format-specifier order (`%s`, `%d`, `%f`, `%c`) exactly as in the msgid; restructure the surrounding Hebrew sentence around the fixed specifier positions rather than reordering specifiers.
