# zh_CN translation rules

## 1. Character set and symbology

- Simplified Chinese characters (简体中文, GB 18030 / Unicode CJK Unified Ideographs) only; never Traditional forms.
- No case distinction; casing/hotkey-shift rules for Latin scripts do not apply to the Chinese text itself, but GTK `_` mnemonic markers stay on a Latin letter appended in parentheses, eg `可视化(_V)`, because Chinese glyphs cannot carry an underline mnemonic.
- Punctuation: use full-width Chinese punctuation in prose (`：`, `，`, `。`, `；`, `？`, `！`, `（）`) around Chinese text; keep ASCII punctuation inside embedded technical tokens (`'%s'`, file paths, card mnemonics) and after Latin abbreviations followed directly by a colon in log-style strings, matching existing file usage (eg `config_sync_entry：`).
- Numbers, units, and symbols stay Western/ASCII and unchanged: `MHz`, `dBi`, `Ω`, `%`, `VSWR`, digits.
- No added spacing convention beyond what already exists in the catalog; do not insert spaces between CJK and adjacent Latin/digits where the existing file omits them.
- Quotation marks: reuse ASCII `'...'` for embedded identifiers/format placeholders (matches existing entries); do not introduce `「」` for these technical tokens.

## 2. Technical interface writing standards

- Menu items, buttons, labels: short noun or verb-object phrases, no trailing punctuation, no filler particles (啊/呢/啦/吧).
- Error/log/debug messages: `<component>：<message>` colon-separated style already established in the file (eg `mem_report:`, `theme:` style source-prefixed messages keep the prefix in English, colon in ASCII, message text in Chinese where translated).
- Dialog/status messages translate fully into natural technical Chinese; keep sentence concise, drop redundant subject pronouns unless directly addressing the user in a confirmation/warning dialog.
- Tooltips: complete, concise sentences describing purpose/reason; no trailing period needed for short phrases, but multi-sentence tooltips keep 。 between sentences.

## 3. Formality mapping

- Direct address to the user (confirmation/warning dialogs, "are you sure" prompts): use 您 (formal), matching existing usage in the catalog.
- Commands, menu items, button labels, and status/log messages: no pronoun (standard Chinese technical-UI convention); do not insert 您/你 where the English source has no direct address.
- Never use 你 (informal) anywhere in the interface.
- Register stays professional/neutral throughout; no colloquialisms, no internet slang.

## 4. NEC2 domain lexicon (established in this catalog — reuse, do not vary)

| English | zh_CN |
|---|---|
| gain | 增益 |
| current | 电流 |
| charge | 电荷 |
| impedance | 阻抗 |
| wire | 导线 |
| segment | 分段 |
| geometry | 几何 |
| excitation | 激励 |
| frequency | 频率 |
| polarization | 极化 |
| patch(es) | 面片 |
| phase | 相位 |
| magnitude | 幅度 |
| scale | 缩放 (color/zoom scale context) |
| flow direction | 流向 |
| wireframe | 线框 |
| total field | 总场 |
| color projection | 色彩投影 (never 颜色投影; applies to labels, tooltips, and log messages alike) |
| color scale / Color Scale (menu) | 色阶 |
| scale family / color family / transfer family (the selectable transfer) | 色阶族 (never 颜色族) |
| brightness | 亮度 |
| animation / animate | 动画 |
| optimizer | 优化器 |
| validation | 校验 |
| feedpoint | 馈电点 |
| radiation pattern | 辐射方向图 |

### Never translate

- NEC2 card mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN, SP, SM, NE, NH
- File extensions: .nec, .csv, .s1p, .s2p, .png
- Unit/technical symbols: VSWR, dBi, MHz, Ω, S-parameters (may render 参数 only if qualifying, but keep "S" untranslated)
- Function/identifier names embedded in developer-facing messages (eg `mem_array_zero`, `config_widget_lookup`, `opt_start`)
