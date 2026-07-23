# th translation rules

## 1. Script, symbology, expectations
- Thai script (ก-ฮ consonants, สระ vowel signs, วรรณยุกต์ tone marks). Thai is LTR, not RTL — the RTL/mirroring guidance in doc/TRANSLATING.md does not apply.
- No uppercase/lowercase distinction; do not attempt to render GTK `_` hotkey case conventions — keep the `_` immediately before the mnemonic character (may be a Thai consonant or a retained Latin letter for a preserved English term).
- Use Arabic numerals (0-9) for all numbers, not Thai numerals (๐-๙). Units, NEC2 mnemonics, file extensions stay Latin/LTR embedded inline (MHz, dBi, Ω, VSWR, .nec, GW, EX, LD, FR, RP, GE, EN).
- Thai does not mark plurals; nplurals=1; plural=0 (already correct in the header).
- Thai orthography does not put spaces between words in running prose, but a space is conventionally inserted between Thai text and an embedded Latin/number token (already the practice throughout this catalog, e.g. "ความถี่แอนิเมชัน (Hz)").
- No terminal sentence period required in Thai; keep msgid's own trailing `\n`/punctuation mechanically, do not add a Thai full stop.

## 2. Technical interface writing standard
- Established loanwords (transliterated) already used consistently in this catalog and to be reused, never re-coined:
  - กระแส = current, ประจุ = charge, เฟส = phase, อิมพีแดนซ์ = impedance, แอนิเมชัน = animation, เธรด = thread, โพลาไรเซชัน = polarization, โครงลวด = wireframe, มาตราส่วน = scale, พื้นผิว = surface, ค่าเริ่มต้น = default, วิดเจ็ต = widget (UI sense; developer/debug messages may keep bare English "widget").
  - อัตราขยาย = gain (native compound, not a transliteration) — use this, not a transliterated "เกน".
- Developer/debug-only strings (pr_debug, raw function-name messages such as `config_widget_*`, `mem_*`, internal assertion text) may keep embedded English identifiers/words verbatim; this matches existing entries and is LOW priority per doc/TRANSLATING.md.
- User-facing UI (menu items, dialog text, tooltips, error dialogs) must be fully rendered in Thai prose around the preserved technical tokens; do not leave ordinary English phrases (e.g. "surface patches") untranslated inside an otherwise-Thai sentence — translate the common words, keep only true NEC2/units tokens (SP/SM, tag, segment, dBi, etc.) in Latin.

## 3. Formality / register (distinct from 1-2: this is about politeness marking, not vocabulary or script)
- Use formal written Thai (ภาษาเขียนทางการ), no colloquial or royal register.
- Thai UI conventionally omits the subject pronoun entirely in commands/menu items (imperative verb only, e.g. "บันทึก" not "คุณบันทึก"); do not insert "คุณ" to render English "you" in instructions.
- Do not add spoken politeness particles (ครับ/ค่ะ/นะ) to menu items, buttons, or terse status/error strings; a fuller informational dialog message may end in a neutral polite frame only if the English source itself is a full sentence (e.g. confirmation dialogs), but default to neutral formal register throughout.

## 4. NEC2/EM domain mapping for Thai
- Never translate: NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), VSWR, dBi, S-parameters, Ω, MHz, degrees, file extensions (.nec, .csv, .s1p, .s2p, .png), "patch"/"tag"/"segment" as NEC2 geometry terms may stay Latin when used as a card-type label, but ordinary sentence words around them (surface, wire, model, card) render in Thai using the vocabulary in section 2.
- "current" → กระแส (electrical sense only, never "ปัจจุบัน" a temporal sense).
- "charge" → ประจุ (electrical sense only, never a billing sense).
- "ground"/"ground plane" → กราวด์/ระนาบกราวด์ (RF reference sense, never "ดิน"/soil).
- "load" → โหลด/อิมพีแดนซ์โหลด (electrical impedance sense, never a burden/weight sense).
- "gain" → อัตราขยาย (antenna directivity ratio, never "กำไร" profit sense).
- "wire" (bare thin conductor, the GW primitive) → ลวด (เส้นลวด for the GW-card element label); never สาย, which leans cable/lead. Reserve สาย only for fixed compounds where it is the standard term: สายส่ง = transmission line, สายอากาศ = antenna. "ground wave" stays คลื่นดิน (propagation term). Apply ลวด uniformly to wire diameter/radius/conductivity, thin-wire kernel (ลวดบาง), and structure-wire contexts.
- Do not append an extra disambiguating qualifier (e.g. "กระแสไฟฟ้า") when the plain established term (กระแส) is already unambiguous in this software's context; only add a qualifier where a specific string would otherwise collide with an unrelated Thai-language meaning.
