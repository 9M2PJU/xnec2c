# zh_TW translation rules

Traditional Chinese (Taiwan). Reference for consistent xnec2c localization.

## Script and punctuation

- No case distinction; no letter-spacing.
- Use full-width ideographic punctuation for Chinese sentences: 。，、；：？！「」
- Numbers, unit symbols, and NEC2 mnemonics stay half-width ASCII: dBi, MHz, Ω, VSWR, %, GW, EX, LD, FR, RP, GE, EN.
- Menu items that open a dialog end with a single-character ellipsis … (U+2026), not three periods.
- Parentheses enclosing English/Latin terms inline with Chinese text stay ASCII `()`.
- GTK hotkey mnemonics use `(_X)` suffix form after the Chinese label (eg `檢視幾何結構(_G)`), matching existing catalog convention.

## Lexicon (Taiwan IT convention, distinct from zh_CN)

| English | zh_TW |
|---|---|
| file | 檔案 |
| edit | 編輯 |
| view | 檢視 |
| settings/preferences | 設定 |
| options | 選項 |
| software | 軟體 |
| network | 網路 |
| program | 程式 |
| thread | 執行緒 |
| memory | 記憶體 |
| loop | 迴圈 |

## Formality

Taiwan technical UI drops the subject pronoun (您/你) entirely; use imperative or topic-comment phrasing instead of addressing the user directly. Do not insert 您 where the English source has no subject.

## NEC2 domain lexicon

| English | zh_TW | Notes |
|---|---|---|
| current | 電流 | electrical, not "recent" |
| charge | 電荷 | electrical, not billing |
| gain | 增益 | antenna directivity |
| impedance | 阻抗 | |
| polarization | 極化 | |
| phase | 相位 | |
| excitation | 激勵 | |
| geometry | 幾何 | |
| frequency | 頻率 | |
| radiation pattern | 輻射場型 / 場型 | |
| patch | 面片 | NEC2 surface patch |
| wireframe | 線框 | |
| scale/zoom | 縮放 | |
| color | 顏色 | |
| flow direction | 流向 | |
| animate/animation | 動畫 | |
| animated (category, adj.) | 動態 | paired with static 靜態; distinct from the noun/verb 動畫 |
| static (category, adj.) | 靜態 | |
| polarity (sign, +/−) | 極性 | distinct from polarization 極化 (EM wave sense); never conflate |
| polarization | 極化 | EM wave polarization only |
| standing wave / traveling wave | 駐波 / 行波 | not literal "standing/moving" |
| node / antinode | 波節 / 波腹 | standing-wave physics terms, not literal "node" |
| far-field contribution | 遠場貢獻 | distinct from "Near Field Animation" 近場動畫 |
| palette | 調色盤 | |
| ramp / gradient | 漸層 | |
| scale family (tone-transfer family) | 刻度家族 | never 顏色家族 |
| comet (wave-crest highlight) | 彗星 | |
| instantaneous (projection name, no phase qualifier) | 瞬時 | distinct from "Instantaneous (φ=0)" 瞬時值 (φ=0), a separate near-field vector selector string |

Never translate: VSWR, dBi, Ω, degrees, NEC2 card mnemonics (GW, EX, LD, FR, RP, GE, EN), file extensions (.nec, .csv, .s1p, .s2p, .png).

## NEC2 "card" convention

- Mnemonic-suffixed card reference: use the concise suffix 卡 (eg `FR 卡`, `EX 卡`, `GC 卡`, `SP/SM 卡`), matching Taiwan compound usage (顯示卡, 網路卡). Never leave the English word "Card" untranslated in a msgstr.
- Standalone noun "Card"/"Cards" (column header, Delete/Manage Card buttons) and generic prose "the card" without a mnemonic: use 卡片 (eg `刪除卡片`, `管理卡片`, `卡片助記碼`).
- Dialog-title qualifier `(XX card)`: ASCII parentheses with a leading space and the 卡 suffix, eg `弧形元件 (GA 卡)`, matching the sibling titles `(NT 卡)`, `(RP 卡)`.
