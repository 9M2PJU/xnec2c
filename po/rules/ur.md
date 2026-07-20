# ur translation rules

## 1. Native character set, symbology, expectations

- Script: Perso-Arabic abjad written RTL, ideally rendered in Nastaliq style; app UI commonly falls back to Naskh-style Arabic-Unicode fonts. No letter case distinction (no uppercase concept).
- Digits: Western Arabic numerals (0-9) are standard in Pakistani technical/engineering software, not Urdu-Indic digits (۰۱۲...). Keep numerals, units, and formulas LTR embedded in RTL sentences per Unicode bidi algorithm.
- Preserve untranslated per project-wide rule: NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), file extensions (.nec, .csv, .s1p, .s2p, .png), unit symbols (MHz, dBi, Ω, deg), VSWR, S-parameters, Z, dB.
- Punctuation: Urdu uses its own comma "،" and question mark "؟" in prose, but code-facing strings (usage/help text, format-specifier messages) keep ASCII punctuation to avoid breaking embedded technical tokens; colons/parentheses stay ASCII since they bound LTR technical runs.
- No word-final ligature breaking concerns for UI labels; keep compound technical phrases short since Urdu script expands ~25% versus English (per doc/TRANSLATING.md).

## 2. Technical program interface writing standards

- Prefer established Urdu engineering/computing loanwords already transliterated into Perso-Arabic script over invented calques: کرنٹ (current), وولٹیج (voltage), فریکوئنسی (frequency), امپیڈینس (impedance), جیومیٹری (geometry), اینیمیشن/اینیمیٹ (animation/animate), ویژوالائزیشن (visualization), آپٹیمائزر (optimizer), تھریڈ (thread), ویجٹ (widget), کنفیگ (config), میموری (memory).
- Menu/button labels: short noun phrases or imperative verb phrases, consistent with existing catalog entries (eg "کرنٹس دیکھیں" style already used for "View Currents").
- Error/debug messages: direct, technical register; keep identifier names, function names (eg `config_widget_lookup`), and printf specifiers untouched; translate only the surrounding descriptive text.
- Hotkey convention: GTK "_X" underscore markers stay on a Latin letter matching the mnemonic already used elsewhere in this catalog for that command family (RTL scripts do not carry a native underline-mnemonic system, so xnec2c keeps the underscore anchored to the corresponding Latin/loanword abbreviation already established in-file, eg "_Asinh", "μ-_law").

## 3. Formality and informality mapping

- Register: formal آپ throughout; never تم or تو. Verb forms use the polite/plural agreement (eg "کریں", "دیکھیں", "منتخب کریں") rather than casual imperative ("کرو").
- No T-V distinction ambiguity in a technical tool audience (engineers/hobbyists) — sections 1-3 stay non-overlapping: §1 is script/symbol mechanics, §2 is lexical/register choice for UI strings, §3 is only the pronoun/politeness level governing verb conjugation.
- Courteous but concise: avoid honorific padding (no "براہ کرم" in every sentence); reserve courtesy markers for user-facing confirmation/error dialogs, not internal debug/log strings (which stay LOW priority, technical register, per doc/TRANSLATING.md).

## 4. Mapping to the NEC2 electromagnetic simulator domain

- Electrical senses per doc/TRANSLATING.md disambiguation table, using established in-file glossary:
  - current → کرنٹ (electrical, Amperes) — never "حالیہ" (recent/present time)
  - charge → چارج (electrical, Coulombs) — never "معاوضہ" (billing)
  - ground/ground plane → گراؤنڈ / گراؤنڈ پلین (electrical reference) — never "زمین" (soil) alone
  - gain → گین (antenna directivity dB) — never "نفع" (profit)
  - load → لوڈ (impedance) — never "بوجھ" (weight) in circuit context
  - pattern (radiation) → پیٹرن — 3D directional response, not "ڈیزائن"
  - excitation → ایکسائٹیشن — EM energy input, not emotional state
  - wire → تار (thin conductor), segment → سیگمنٹ, patch → پیچ, tag → ٹیگ (kept close to NEC2 term)
- Do not add qualifiers absent from the English source (eg "کرنٹس دیکھیں" not "برقی کرنٹس دیکھیں") — program context (an EM simulator) already disambiguates for the domain-expert user, matching the Afrikaans/German/Spanish examples in doc/TRANSLATING.md.
- Keep NEC2 card mnemonics, unit symbols, and file extensions untranslated and LTR inside RTL sentences.
- polarity (sign of a quantity, eg current/charge diverging-hue sign) → قطبیت — distinct from polarization (antenna field orientation) → پولرائزیشن; do not conflate: msgid "Polarity" is not msgid "_Polarization".

## 5. Established in-catalog glossary (for consistency)

| English | Urdu |
|---|---|
| current(s) | کرنٹ / کرنٹس |
| charge(s) | چارج / چارجز |
| magnitude | شدت / مقدار |
| direction | سمت |
| phase | فیز |
| reference phase | حوالہ فیز |
| wireframe | وائرفریم |
| polarization | پولرائزیشن |
| axis | محور |
| peak | چوٹی |
| value | قدر |
| patch(es) | پیچ |
| total field | کل فیلڈ |
| visualization | ویژوالائزیشن |
| scale | پیمانہ |
| linear | لینیئر |
| power | پاور |
| draw style | ڈرا اسٹائل |
| level | سطح |
| invalid | غلط |
| using ... | ... استعمال کر رہا ہے |
| missing widget | غائب ویجٹ |
| unknown widget type | نامعلوم ویجٹ قسم |
| geometry | جیومیٹری |
| ignoring ... | ... نظرانداز کیا جا رہا ہے |
| data card | ڈیٹا کارڈ |
| excitation | ایکسائٹیشن |
| frequency | فریکوئنسی |
| loop | لوپ |
| save | محفوظ |
| surface patches | سطحی پیچ |
| near-field | نزدیکی فیلڈ |
| required | درکار |
| memory allocation | میموری ایلوکیشن |
| failed | ناکام |
| optimizer | آپٹیمائزر |
| output thread | آؤٹ پٹ تھریڈ |
| color projection | کلر پروجیکشن |
| color family / scale family | کلر فیملی / اسکیل فیملی |
| gamma | گاما |
| dynamic range | ڈائنامک رینج |
| compression | کمپریشن |
| contrast | کنٹراسٹ |
| knee | نی |
| brightness | برائٹنس |
| render / rendering | رینڈر / رینڈرنگ |
| theme | تھیم |
| feedpoint | فیڈ پوائنٹ |
| port | پورٹ |
| validation | ویلیڈیشن |
| batch mode | بیچ موڈ |
| config / configuration | کنفیگ / کنفیگریشن |
| polarity (sign) | قطبیت |
| polarization | پولرائزیشن |
| palette | پیلیٹ |
| color tone | کلر ٹون |
| comet (overlay style) | کومٹ |
| electric field | برقی فیلڈ |
| magnetic field | مقناطیسی فیلڈ |
| radiation | ریڈی ایشن |
| far-field | فار فیلڈ |
| overlay(s) | اوورلے / اوورلیز |
| node/antinode | نوڈ / اینٹی نوڈ |
