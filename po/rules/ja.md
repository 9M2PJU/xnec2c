# ja translation rules

## 1. Native character set, symbology, and expectations

- Script mix: Kanji (漢字) for content words, Hiragana (ひらがな) for grammatical
  particles/inflection, Katakana (カタカナ) for loanwords and technical/foreign
  terms (eg アニメーション "animation", ベクトル "vector", レンダリング
  "rendering", オプティマイザ "optimizer").
- No word spacing; no letter case distinction (no upper/lower case concept).
- Long-vowel mark ー extends katakana loanword vowels (eg ビューア "viewer").
- Half-width (ASCII) digits, Latin letters, and symbols are used for numerals,
  units, file paths, and NEC2 mnemonics embedded in Japanese text (eg
  "10 MHz", "GW", "VSWR"), matching existing catalog usage. Do not switch
  these to full-width forms.
- Punctuation: use the Japanese full stop `。` and comma `、` only inside full
  natural-language sentences (eg dialog/tooltip prose). Short UI labels,
  menu items, and log/error strings copy the source punctuation style
  (colons, parentheses, quotes) rather than inserting `。`.
- Retain Greek/Latin unit symbols and technical symbols unchanged: Ω, %, °,
  dBi, MHz, γ, φ, μ (eg μ-law stays "μ-law" per existing "μ-_law" entry).
- Mnemonics and identifiers (GW, EX, LD, FR, RP, VSWR, S-parameters, file
  extensions .nec/.csv/.s1p/.s2p/.png) are never translated or transliterated.

## 2. Writing standards for a technical program interface

- User-facing prose (dialog messages, tooltips, confirmation text) uses
  polite です/ます sentence form, per doc/TRANSLATING.md formality guidance.
- Menu items, button labels, and short field labels are noun phrases, not
  full sentences, matching existing entries (eg "スケール" for "Scale",
  "パッチ" for "Patch", not verb-conjugated sentences).
- Log/console/error strings (`pr_*`, `%s:` prefixed diagnostics) stay
  concise and descriptive, ending in plain-form phrasing rather than
  polite です/ます, matching existing entries like
  "ウィジェット '%s' が見つかりません" (not "見つかりませんでした。").
  This matches TRANSLATING.md's LOW priority for developer/debug strings:
  correctness and clarity over conversational politeness.
- Katakana loanwords are preferred over invented kanji compounds for
  established computing/engineering terms already used in the catalog
  (アニメーション, レンダリング, ジオメトリ, オプティマイザ, ビューア,
  スミス "Smith", スケール "Scale").
- GTK mnemonic underscores (`_X`) map to a parenthetical katakana/kana
  reading marker `(_X)` placed after the translated label, matching
  existing entries (eg "偏波軸(_A)", "瞬時値(_I)(φ=0)", "電流可視化(_V)").
  Choose the mnemonic letter from the *source* accelerator letter already
  used elsewhere for the same or a related concept when one exists, to
  avoid duplicate mnemonics within one menu/dialog.

## 3. Formality and informality mapping

- Formality (public/professional register): です/ます polite forms in all
  user-facing dialogs, tooltips, and messages. This is the only register
  used in this catalog — xnec2c is professional engineering software with
  no informal/casual variant.
- Informality (だ/である plain, casual speech): never used for user-facing
  text; explicitly excluded per doc/TRANSLATING.md ("avoid casual だ/である
  forms").
- Plain descriptive form (no polite copula, no casual copula) is reserved
  for developer-facing log/error strings per §2, a distinct register from
  both 1 and 2 above — it is terse technical reportage, not casual speech.
- These three registers (polite user-facing, excluded casual, plain
  developer log) do not overlap in scope: user-facing strings never use
  plain-log style, and log strings never use です/ます.

## 4. NEC2 electromagnetic-simulator domain mapping (ja)

Established lexicon already present in this catalog — reuse these exactly,
do not introduce synonyms:

| English | Japanese | Notes |
|---|---|---|
| Currents | 電流 | electrical current, not "current time" |
| Charges | 電荷 | electrical charge |
| Gain | 利得 | antenna directivity ratio, not "profit" |
| Geometry | ジオメトリ | NEC2 model geometry |
| Excitation | 励振 | EX card excitation |
| Radiation | 放射 | radiation pattern |
| Frequency | 周波数 | FR card frequency |
| Wire | ワイヤ | NEC2 wire segment conductor |
| Patch | パッチ | NEC2 surface patch (SP/SM) |
| Viewer | ビューア | rendering viewer |
| Smith | スミス | Smith chart, kept as loanword |
| VSWR | VSWR | never translated |
| Total field | 全界 | radiation total-field |
| Peak value | ピーク値 | katakana ピーク + 値 |
| Wireframe | ワイヤフレーム | rendering mode |
| Polarization axis | 偏波軸 | 偏波 = polarization |
| Flow direction | フロー方向 | animated current/patch flow |
| Reference phase | 基準位相 | animation reference phase |
| None (geometry display) | ジオメトリを表示 | context: "show geometry" phrasing already established |
| Instantaneous (φ=0) | 瞬時値(φ=0) | instantaneous value, φ retained unchanged |
| Magnitude | 大きさ | modulus of a quantity (\|Z\|, current, charge); never 振幅 (that is amplitude, a distinct concept and wrong for \|Z\|) |
| Net Gain | 正味利得 | net gain; never ネットゲイン — keep kanji 正味利得 as in graph titles |
| Structure Geometry | 構造ジオメトリ | geometry stays ジオメトリ per Geometry row, not 形状 |
| Polarity (sign, +/−) | 極性 | instantaneous sign of a displayed quantity; distinct from 偏波(_P) "_Polarization" (antenna wave polarization) — never conflate |
| Far-field | 遠方界 | antonym of established 近傍界 "Near field"; eg "Far-field Contribution" → 遠方界寄与 |
| Amplitude | 振幅 | distinct from Magnitude 大きさ (see Magnitude row); used only where source literally says "amplitude" |
| Animated / Static (projection category) | アニメーション / 静的 | non-interactive menu group headers pairing dynamic vs phase-invariant projections |
| Node / Antinode | 節 / 腹 | standing-wave zero/maximum points; standard Japanese physics terms, not transliterated |
| Comet (overlay effect) | コメット | katakana loanword, matching established transliteration style for effect/algorithm names (Reinhard, Sigmoid, Asinh) |
| Ground (earth reference / ground plane) | グラウンド (eg グラウンドプレーン) | electrical earth reference, not soil; always the full transliteration グラウンド, never the contracted グランド — apply catalog-wide (eg グラウンド効果, グラウンドパラメータ) |

Unit symbols, NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN), file
extensions, and Ω/dBi/MHz/VSWR/S-parameters/γ/φ/μ stay in Latin/Greek form
per §1 and doc/TRANSLATING.md.

Disambiguation follows doc/TRANSLATING.md: choose the electrical/RF sense
for ambiguous terms (charge, current, ground, wire, gain, pattern,
excitation, load) without adding a qualifier absent from the English
source — eg 電流 alone for "Currents", not 電気電流 or 電流量.
