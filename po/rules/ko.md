# ko translation rules

## 1. Script, symbology, and expectations

- Script: Hangul (한글) syllable blocks; no uppercase/lowercase distinction, so English casing conventions (title case, sentence case) do not apply to Korean text itself.
- Word spacing (띄어쓰기): space between independent words/particles-attached nouns as per standard Korean orthography; do not remove spacing to save width unless the term is a fixed compound.
- Punctuation: use Korean-standard full stop `.`, comma `,`; do not substitute ideographic full stops (`。`). Keep half-width Latin punctuation for technical notation.
- Numbers, units, and NEC2 mnemonics stay LTR and unchanged: `MHz`, `dBi`, `Ω`, `%`, `VSWR`, `GW`, `EX`, `LD`, `FR`, `RP`, `GE`, `EN`, file extensions (`.nec`, `.csv`, `.s1p`, `.s2p`, `.png`).
- Loanwords (외래어): technical terms without a native Korean equivalent are transliterated into Hangul (e.g., 임피던스 impedance, 벡터 vector, 텍스처 texture) rather than translated semantically; prefer the term already established in Korean engineering usage.
- Parentheses/colons/quotes: keep half-width `()`, `:`, and reuse straight quotes `'…'` as in source for placeholder values (e.g., widget names inside `%s`).

## 2. Technical interface writing standards

- Menu items, buttons, and labels: concise noun-phrase form (명사형), e.g. "저장", "설정", not full sentences.
- Dialog messages and errors: formal declarative sentences ending in 합니다/습니다 (hapsyoche), not colloquial 해요체 and never plain/banmal (반말) 이다/한다 endings.
- Debug/developer-facing pr_debug-level strings: low translation priority per TRANSLATING.md; keep faithful literal technical meaning, formal register still applies if translated.
- Tooltips: full declarative sentences in 합니다 form describing what the control does and, when disabled, why.
- Hotkey mnemonics (`_X` GTK underscore accelerators): choose a Korean-typable Latin letter tied to the loanword/romanization already used in the string when the label mixes Hangul and a bracketed Latin mnemonic is not used in this catalog; when the source uses `_Letter` on an English/loan term embedded in the Korean string, preserve that same letter's position in the Korean msgstr's embedded Latin term.

## 3. Formality mapping

- Buttons, menu commands, and dialog action labels: 합니다체 register in surrounding sentences; command itself remains a noun phrase per §2.
- Status/info/error messages: 습니다/합니다 formal declarative.
- Less formal secondary UI text (inline hints) may use 해요체 per TRANSLATING.md guidance, but default to 합니다 unless an existing string in this catalog already establishes 해요 for that exact widget class.
- Never use 반말 (banmal) or plain-form 이다/한다 verb endings anywhere in the interface.

## 4. NEC2 electromagnetic-simulation domain mapping (Korean)

| English | Korean | Notes |
|---|---|---|
| current | 전류 | electrical current (Amperes), never "현재/최근" |
| charge | 전하 | electrical charge (Coulombs), never billing |
| ground / ground plane | 접지 / 접지면 | RF reference plane, never soil; use 접지면 for "ground plane", 접지 for bare "ground/grounding"; 지면 only for the physical earth/horizon (eg sky/ground boundary) |
| wire | 도선 | thin conductor, never cable/cord (never 전선/케이블) |
| field | 필드 / 장 | use 장 in Korean-word compounds (전기장, 자기장, 근접장=near field, 원거리장=far-field); use 필드 after a Latin symbol (E 필드, H 필드) or bare "field" |
| gain | 이득 | antenna directivity ratio, never profit |
| pattern (radiation) | 패턴 | 방사 패턴 when qualifying "radiation pattern"; keep bare 패턴 when source is bare |
| excitation | 여기 | electromagnetic energy input, never emotional state |
| load / impedance | 부하 / 임피던스 | electrical impedance, never weight |
| radials | 방사형 도선 | ground-plane radial wires |
| segment / patch / tag | 세그먼트 / 패치 / 태그 | NEC2 geometry terms, transliterated, not translated |
| impedance | 임피던스 | loanword, standard EE usage |
| projection (color) | 투영 | color-mapping projection |
| scale | 스케일 | loanword, standard usage already in this catalog |
| flow (current/patch flow) | 흐름 | direction/animation flow |
| wireframe | 와이어프레임 | loanword, standard usage |
| smith (chart) | 스미스 (차트) | keep as loanword/proper noun |

## 5. Disambiguation policy

- Use the correct technical sense from §4 without adding qualifiers absent from the English source (eg "전류" alone for "View Currents", not "전기 전류").
- Add a qualifying word only when Korean usage would otherwise be ambiguous within the specific UI context (eg distinguishing "여기" excitation from other unrelated senses only if genuinely ambiguous in that string).
- excitation is 여기 catalog-wide, but 여기 also spells the locative "here"; never let both senses collide in one string. When a string needs the locative "here" alongside excitation, reword the locative (eg "이 목록에" = in this list, "여기서" only when no excitation term shares the string) so the sole remaining 여기 reads as excitation.
