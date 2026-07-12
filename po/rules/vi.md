# vi translation rules

## 1. Native character set, symbology, expectations

- Latin script (quốc ngữ) with diacritics: base letters plus ă, â, ê, ô, ơ, ư, đ; five tone
  marks (sắc, huyền, hỏi, ngã, nặng) plus unmarked (ngang). UTF-8 required; never strip
  diacritics or tone marks — doing so changes word meaning (eg `ma` vs `mà` vs `má`).
- Vietnamese is analytic: no verb conjugation, no grammatical gender, no plural inflection.
  Do not invent plural markers; "1 tệp" / "5 tệp" both use the bare noun.
- Numbers inside format specifiers (`%d`, `%f`, etc.) are produced by the C runtime, not by
  the translated string; never alter their formatting or add a Vietnamese decimal comma —
  only the surrounding prose is translated.
- Preserve unchanged, per doc/TRANSLATING.md: NEC2 mnemonics (GW, GA, GH, EX, LD, FR, RP, GE,
  EN), VSWR, dBi, Ω, MHz, S-parameters, file extensions (.nec, .csv, .s1p, .s2p, .png).

## 2. Technical interface writing standards

- Vietnamese OSS/technical convention favors impersonal, subject-dropped imperative phrasing
  for menu items, buttons, and status text (eg "Không thể lưu tệp" not "Bạn không thể lưu tệp").
  Reserve explicit pronouns for direct user-facing questions (confirmation dialogs).
- Sino-Vietnamese (Hán Việt) compounds are the normal register for technical/scientific
  vocabulary (eg trực quan hóa, phân cực, khuếch đại, tần số) — prefer these over colloquial
  paraphrase.
- Capitalization: only the first word of a sentence/label and proper nouns; no English-style
  title case on multi-word menu labels (eg "Hiển thị bề mặt" not "Hiển Thị Bề Mặt").
- Keep sentences compact; Vietnamese technical prose trends shorter than English source due to
  monosyllabic root words compounding efficiently.
- GTK hotkeys (`_X`): place the underscore on a syllable-initial consonant/vowel that is
  distinctive within the dialog to avoid collisions; diacritics are retained under the
  underscore (eg `_Tần số`).

## 3. Formality mapping

- Default register: impersonal/subject-dropped (see §2) — this is the neutral technical
  register, distinct from any specific pronoun choice.
- When a pronoun is unavoidable (direct confirmation questions), use "bạn": neutral, polite,
  peer-level, standard in Vietnamese OSS localization (GNOME, KDE, Ubuntu Vietnamese use
  "bạn"). Example: "Bạn có chắc chắn muốn thoát xnec2c không?"
- Do not use "quý khách" (commercial/customer-service formal) — wrong register, implies a
  vendor-customer relationship absent in engineering tools.
- Do not use "ông/bà" (elder/high-formal address) — overly distant for a technical utility;
  inappropriate register mismatch with peer engineers/hobbyists.
- Sections 1-3 do not overlap: §1 is orthography (script/diacritics/units), §2 is sentence
  construction (word order, register of vocabulary, capitalization), §3 is pronoun/address
  selection only.

## 4. NEC2 domain mapping

| English | Vietnamese | Notes |
|---|---|---|
| current (electrical) | dòng điện | never "hiện tại" (present time) |
| charge (electrical) | điện tích | never "phí" (billing) |
| ground / ground plane | mặt đất (RF reference) / mặt phẳng đất | not "đất" alone (soil) when ambiguous |
| wire | dây (dẫn) | thin conductor; not "cáp" (cable/cord) |
| gain | độ lợi / khuếch đại | antenna directivity (dB); never "lợi nhuận" (profit) |
| radiation pattern | giản đồ bức xạ | not "mẫu"/"khuôn" (template/mold) |
| excitation | kích thích | EM energy input; no emotional sense in Vietnamese anyway |
| load (impedance) | tải | not "gánh nặng" (burden) |
| segment | đoạn (dây) | NEC2 geometry term |
| patch | mảnh | NEC2 geometry term (surface patch) |
| tag | thẻ (đánh dấu hình học) | do not confuse with "thẻ dữ liệu" (data card) |
| command/data card | thẻ (lệnh/dữ liệu) | NEC2 card mnemonic carrier, eg "thẻ FR" |
| geometry | hình học | |
| polarization | phân cực | |
| visualization | trực quan hóa | |
| animation | hoạt ảnh | prefer over "hoạt hình" (cartoon-flavored) for consistency; sweep file for stray "hoạt hình" |
| impedance | trở kháng | |
| feedpoint | điểm cấp điện | |
| projection | phép chiếu | |
| scale | tỷ lệ | |
| total field | trường tổng | |
| peak value | giá trị đỉnh | |
| reference phase | pha tham chiếu | |
| wireframe | khung dây | |

## 5. Consistency notes

- One Vietnamese term per English concept across the whole catalog (priority c in frame.md);
  "hoạt ảnh" is the chosen term for "animate/animation" — reconcile any "hoạt hình" found
  elsewhere in the catalog to "hoạt ảnh" during the consistency sweep.
- Disambiguation qualifiers (eg "điện" appended to "dòng điện") are added only where the
  Vietnamese noun alone is genuinely ambiguous outside this domain; do not add qualifiers the
  English source omits (see doc/TRANSLATING.md Context-Dependent Disambiguation).
- Sentence case (§2) applies to graph titles and plain labels: English title case is not
  mirrored, so a mid-phrase prepositional term stays lowercase (eg "vs Frequency" → "theo tần
  số", never "theo Tần số"; "& Net Gain" → "& độ lợi ròng"). Only referenced UI-element names
  (a named button/menu target the text points the user to, eg "nút Dòng điện", "Chọn Kiểu Độ
  lợi → …") keep their label capitalization.
