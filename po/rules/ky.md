# ky translation rules

## 1. Native character set, symbology, expectations

- Script: Kyrgyz Cyrillic (Kyrgyzstan). Alphabet is the Russian Cyrillic set
  plus three Kyrgyz-specific letters: **Ң** (ng), **Ө** (o-umlaut), **Ү**
  (u-umlaut). Loan letters В, Ф, Ц, Щ, Ъ, Ь, Ё appear only in established
  loanwords; do not substitute them into native Kyrgyz roots.
- Case: full upper/lower case distinction, as in Russian. Preserve GTK
  hotkey underscore casing rules (see below); no separate case convention
  for Kyrgyz technical labels beyond normal sentence/title case already
  used in the catalog (existing entries favor Title Case for menu/button
  labels, sentence case for messages).
- Numerals: Arabic numerals. Runtime numeric values arrive via `%d/%f/%g`
  format specifiers and are locale-rendered by libc, not by the msgstr
  text; do not hand-format numbers inside translated strings. Where a
  literal decimal appears in source prose (none currently in this
  catalog), use comma as the decimal mark per Cyrillic-script convention.
- Units and symbols stay unchanged and LTR: MHz, Hz, dBi, dB, Ω, °, %,
  VSWR, S11, S-parameters. Never transliterate these to Cyrillic (not
  "МГц", "Гц", "дБ"). Physical field symbols **E** and **H** (electric /
  magnetic field, eg "Near E field", "Near _E Field") stay Latin — a
  Cyrillic "Н" would read as "N" and "Э" loses the standard notation;
  match the spelled-out gnuplot strings which keep Latin E/H. This
  applies to compound labels too: "E/H Fields" = "E/H Талаалары" (never
  a Cyrillic "Э/Н"), and "E-field"/"H-field" prefixes stay Latin
  ("E-талаа", "H-талаа"). Also note "near field" is always the EM
  **талаа**, never "аймак" (= region/area). The
  antenna Front/Back ratio abbreviation **F/B** stays Latin everywhere
  (both the graph label and the port tooltip), never a Cyrillic calque
  such as "А/К". NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN)
  and file extensions (.nec, .csv, .s1p, .s2p, .png) are never translated.

## 2. Technical interface writing standards

- Kyrgyz computing/engineering vocabulary is built mostly from
  Russian-derived Cyrillic loanwords carried directly into Kyrgyz
  (виджет, файл, интерфейс, меню, импеданс, поляризация, сегмент, тег,
  геометрия, патч, масштаб). Use the loanword when it is already the
  established term in this catalog (see lexicon below); do not invent a
  purist Kyrgyz neologism where a loan term is already standard and
  understood by the target audience (engineers, radio amateurs).
- Compound technical noun phrases follow Kyrgyz genitive/possessive
  agglutination (eg "Ток булагы" = "current's source" = "Current
  source"), not literal English word order.
- Keep messages terse and neutral, matching existing catalog style;
  avoid literary flourishes in status/error strings.

## 3. Formality mapping

- Kyrgyz marks formality through the сен (informal) / сиз (formal,
  singular-respect or plural) distinction and through verb mood.
- Professional engineering software uses **сиз-register or impersonal
  imperative/infinitive forms** — never сен. Existing catalog imperative
  strings use bare imperative/infinitive verb stems with no explicit
  pronoun (eg "сактоо" = "to save/saving", "көрүү" = "to view/viewing"),
  which read as neutral-formal in Kyrgyz technical writing. Continue this
  pattern: prefer impersonal verbal-noun or imperative-stem phrasing over
  an explicit "сиз" pronoun, matching existing menu/dialog strings.
- Sections 1-3 do not overlap: (1) is script/orthography, (2) is
  technical lexical register, (3) is only the addressee-formality axis.

## 4. NEC2 electromagnetic-simulator domain mapping

Established lexicon already in this catalog (reuse these exactly; do not
introduce synonyms for the same concept):

| English | Kyrgyz | Notes |
|---|---|---|
| current (electrical) | ток | never "учур/азыркы" (temporal "current") |
| charge (electrical) | заряд | |
| power | кубат / кубаттуулук | |
| frequency | жыштык | |
| field (EM) | талаа | never "талаа" = agricultural field sense, disambiguated by domain |
| impedance | импеданс | |
| polarization | поляризация | |
| scale | масштаб | |
| peak | чоку | |
| reference | эталондук (adj) / эталон (n) | |
| segment/tag/patch | сегмент / тег / патч | NEC2 geometry terms, transliterated, not translated to unrelated words; spell tag **тег** with е (never "тэг"), matching the geometry-error messages and the "NEC тег" tooltip |
| batch mode | топтомдук режим | batch/bundle sense; use "топтомдук" everywhere, never the Kazakh-style loan "пакеттик режим" |
| endpoint | уч (чекит) | segment tip/endpoint; "endpoint distance" = "уч аралыгы", never "учур" (moment) — уч ≠ учур |
| geometry | геометрия | |
| widget | виджет | |
| gain | күч / жапыруу коэфф. — use "күч" only in existing "Сызыктуу _Кубат" style contexts; prefer keeping "gain" contextually as "күч" when paired with dB units already established (eg "Макс. күч") |
| ground plane | жер тегиздиги / жерге туташуу тегиздиги | electrical ground reference, not soil |
| excitation | козгоо | electromagnetic excitation source, not emotional state |
| load (impedance) | жүктөм / жүк | electrical impedance load |
| radials | радиалдар | horizontal ground-plane wires |
| wire | зым | thin conductor |
| validation | текшерүү / валидация | do not confuse with "козгоо" (excitation) |
| polarity (sign, +/−) | полярдуулук | electrical/color-sign polarity; never "поляризация" (that is wave polarization, a distinct concept — see "polarization" row above) |
| hue | өң | color-wheel hue channel, eg "Фаза өңү" (Phase Hue); "hue encoding" = "өң кодировкасы" |
| brightness | жарыктык | color-channel brightness/luminance; "brightness encoding" = "жарыктык кодировкасы" |
| near field / far field | жакын талаа / алыс талаа | near/far antenna field region, opposed pair; "far-field contribution" = "алыс талаа салымы" |
| overlay (UI element) | каптама | rendering/geometry overlay layer, eg "Failed to load overlay shaders" = "Каптама шейдерлерин жүктөө ийгиликсиз"; the verb "to overlay [the structure]" uses "кабаттоо" instead (different grammatical role, not a synonym conflict) |
| palette | палитра | color palette; "palette kind" = "палитра түрү" |
| ramp (palette/gradient) | тилке | linear gradient strip, matches existing "color strip" = "түс тилкеси" |
| comet (trail effect) | комета | visual comet-head/trail overlay, not "геометрия" |
| identity (transfer function) | тождик | mathematical identity/no-op transform, distinct from "identity" as personal identity (not present in this domain) |
| companding | компандинг | telephony-style bounded log curve (μ-law), transliterated loanword |
| swap (exchange) | алмаштыруу | verbal-noun; never the Kazakh-style "ауыштыруу" (Kyrgyz root is алмаш-) |
| crossed (transmission line) | кайчылаш | conductors crossed/reversed; never "кесилген" (= cut/severed) |
| viewer (observation direction) | көрүүчү | "Viewer Gain" = "Көрүүчү күч"; use everywhere, not the longer "карап туруучу" |
| radiation pattern | нурланма диаграммасы | use "нурланма" for the pattern noun everywhere; never the process-noun "нурлануу" for this feature |

Disambiguation follows program context; do not append an "electrical"
qualifier absent from the English source (eg "Токторду көрүү" = "View
Currents", not "Электр токторун көрүү").

## 5. Casing and hotkeys

- Latin axis names X/Y/Z stay uppercase (Kyrgyz has no native convention
  overriding this in a technical UI).
- GTK `_` mnemonic accelerators: place the underscore before a Kyrgyz
  letter that is both distinctive within the same dialog/menu and
  reachable on a standard Kyrgyz/Cyrillic keyboard; shift the underscore
  to avoid duplicate accelerators within one container, matching existing
  catalog choices already present in the file (eg "_Жыштык графиктери",
  "Ток _Визуализациясы", "_Поляризация", "Чоку _Чоңдугу").

## 6. Format specifiers

Preserve every `%s %d %f %c %%` token from the msgid, same order, in the
msgstr; Kyrgyz word order is flexible enough that specifiers rarely need
`%N$s` positional reordering, but use it if a fluent rendering demands
reordering.
