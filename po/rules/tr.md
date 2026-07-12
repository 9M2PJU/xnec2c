# Turkish (tr) Translation Rules for xnec2c

## Script & Typography
- Latin-based Turkish alphabet (29 letters): includes ç, ğ, ı (dotless), i (dotted),
  ö, ş, ü. Never confuse dotted İ/i with dotless I/ı when casing mnemonic letters.
- Arabic numerals (0-9) for all technical/measurement values.
- Decimal comma in Turkish prose (`50,0`), but numbers embedded via format
  specifiers (`%d`, `%f`, `%g`) are runtime-formatted; do not alter specifier or
  add locale punctuation around them in the string itself.
- LTR script; standard Western punctuation.

## GTK Mnemonics (_X accelerators)
- Keep one underscore-prefixed mnemonic letter per label, chosen from the Turkish
  translation itself (not transliterated from English) so it stays genuinely
  typable and mnemonic for a Turkish keyboard.
- Prevent duplicate accelerators in the same window/menu by shifting the
  underscore to a different letter in the Turkish word; never invent a letter
  absent from the translated text.

## Formality / Register
- Register splits by string type, matching the existing catalog:
  - Full-sentence tooltips, instructions, and confirmation dialogs use formal
    `siz`-register imperatives (`-in/-ın/-un/-ün` verb suffix, e.g. "Seçin",
    "gösterin", "açarak etkinleştirin", "Bakın", "...emin misiniz").
  - Short button/menu command labels use the bare imperative stem, as the
    established catalog does (e.g. "Sıfırla" = Reset, "Kapat" = Close,
    "Uygula" = Apply, "Ters Çevir" = Invert, "Görüntüle" in "Yükleri
    Görüntüle" = View Charges, "Çalıştır" = Execute). Do not "correct" these
    short labels to `-in` forms; that would contradict the locked glossary
    precedent and the whole catalog.
- Menu items and labels: sentence case, capitalize only the first word and
  proper nouns/acronyms (NEC2, VSWR); Turkish does not capitalize common nouns
  mid-phrase the way German does.
- Dialog confirmation questions use the formal 2nd person plural verb ending
  (`-manız`/`-eceğinizden emin misiniz` pattern) already seen in the catalog,
  e.g. "xnec2c'den çıkmak istediğinizden emin misiniz?".

## Terminology Glossary (lock these translations everywhere)
| English (sense) | Turkish | Notes |
|---|---|---|
| current (electrical, Amperes) | akım | never "şu anki" (temporal) |
| charge (electrical, Coulombs) | yük | existing catalog precedent (`Charges` → `Yükler`, `View Charges` → `Yükleri Görüntüle`); same root also covers "Load/Loading" (LD card) — accepted existing overload, keep consistent, do not invent a separate qualified term |
| load / loading (LD card, impedance) | yük / yükleme | "Yük Türü" = Loading Type, "Yük Komutu" = Loading Command (established) |
| ground plane (RF reference) | toprak düzlemi | not "toprak" alone (soil) |
| ground (GN/GD electrical) | toprak | |
| gain (antenna directivity, dB) | kazanç | never "kâr" (profit) |
| wire (conductor) | tel | not "kablo" (cable/cord) |
| impedance | empedans | |
| pattern (radiation) | örüntü | 3D directional response, not "şablon" (template) |
| excitation | uyarım | EM energy input, not "heyecan" (emotional excitement) |
| segment (NEC2 geometry) | segment | established loanword, do not translate |
| patch (NEC2 geometry) | yama | |
| tag (NEC2 identifier) | etiket | |
| frequency | frekans | |
| phase | faz | |
| magnitude | büyüklük / genlik | "büyüklük" for scalar magnitude, "genlik" for waveform amplitude; keep per established use at each site |
| impedance magnitude (\|Z\|) | büyüklük | scalar magnitude → "büyüklük" (e.g. "Z-magn" = "Z-büyüklük", "Empedans (büyüklük/faz)"); never "genlik" here |
| amplitude (waveform) | genlik | e.g. color projection "using amplitude" = "genlik kullanılıyor" |
| major axis / minor axis (polarization ellipse) | büyük eksen / küçük eksen | geometric ellipse-axis standard; never "ana/yan eksen"; apply everywhere (RP normalization options and cmnd_edit header) |
| instantaneous | anlık | |
| visualization | görselleştirme | |
| animate | canlandır | |
| color projection | renk projeksiyonu | |
| scale (color/graph) | ölçek | as a noun; "ölçekle" only for the imperative verb sense |
| brightness | parlaklık | |
| contrast | kontrast | |
| compression | sıkıştırma | |
| range (dynamic range, dB) | aralık | not "açı" (angle) |
| port (excitation/feed port) | port | established loanword |
| file | dosya | |
| window | pencere | |
| settings | ayarlar | |
| error | hata | |

## Never Translate
- NEC2 mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN, GC, GM, GX, GR, GS, GN, GD, NE, NH,
  NT, TL, EK, KH, XQ, SY, SP, SC, SM, CM, CP, PQ, PT
- Units/symbols: MHz, dBi, Ω, VSWR, dB, S-parameters
- File extensions: .nec, .csv, .s1p, .s2p, .png, .gplot

## Disambiguation Policy
- Use the correct technical sense per glossary above without appending extra
  qualifying words absent from the English source; the EM-simulator context
  disambiguates for the RF-engineer audience.
- Exception: "yük" (charge) vs "yük" (load/loading) is a genuine intra-domain
  homonym already established in this catalog; keep both uses as-is rather than
  inventing a new disambiguating term, since existing entries already rely on
  program context (button/card name) to disambiguate.

## Punctuation & Connectives
- Graph/plot titles keep the compact `&` symbol as-is (e.g. "Maksimum Kazanç &
  Net Kazanç", "VSWR & S11", "G/Ta & TA"); do not spell it "ve" in a title.
- Natural-language dropdown/menu enumerations spell `&` as "ve" (e.g. "Büyük ve
  Küçük Eksen ve Toplam Kazanç", "Dikey, Yatay ve Toplam Kazanç").
- `vs` in chart titles renders as " - " (e.g. "Empedans - Frekans"); keep this
  consistent across all frequency-plot titles.

## Format Specifiers
- Preserve `%s`, `%d`, `%f`, `%c`, `%%`, `%g`, `%lu`, `%llu` etc. in identical
  order to the source. Turkish is agglutinative/SOV; restructure the sentence
  around the fixed specifier positions rather than reordering specifiers.
- When a format specifier is directly followed by a Turkish case/possessive
  suffix, separate with an apostrophe per Turkish orthography (e.g. `%s'yi`,
  `%d'e`) only where grammatically required by existing catalog convention.

## Numbers / Locale
- Decimal point as in source strings (program-formatted numbers keep `.`);
  do not alter numeric format specifiers.
