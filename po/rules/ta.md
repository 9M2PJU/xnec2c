# Tamil (ta) Translation Rules for xnec2c

## Script & Typography
- Tamil script (U+0B80–U+0BFF). No uppercase/lowercase concept.
- Use Arabic numerals (0-9) for all technical/measurement values. Never use Tamil numeral glyphs.
- Western punctuation (., ,, :, ?, !, "") is standard in Tamil software UI.
- LTR script; no bidi handling needed.

## GTK Mnemonics (_X accelerators)
- Tamil glyphs cannot serve as keyboard accelerators. Keep the original Latin mnemonic
  letter in parentheses appended after the Tamil label, e.g. `திற (_O)` for `_Open`.
- When two labels in the same window would collide on the same Latin letter, choose a
  different Latin letter from the English source word (or a synonym) — never invent a
  Tamil-letter accelerator.

## Formality / Register
- Use literary/standard Tamil (செந்தமிழ்) register only. No colloquial spoken forms.
- Avoid direct 2nd-person pronouns (நீங்கள்/நீ) in UI strings. Prefer the polite
  impersonal imperative suffix -கவும் (e.g. சேமிக்கவும் = "Save"/"please save") or
  noun-form labels for buttons/menus/titles.
- Dialog questions (confirmations) may use நீங்கள் ... விரும்புகிறீர்களா? (formal "do you
  wish to...") pattern since it is a direct question to the user.

## Terminology Glossary (lock these translations everywhere)
| English (sense) | Tamil | Notes |
|---|---|---|
| current (electrical, Amperes) | மின்னோட்டம் | never நடப்பு/தற்போதைய (temporal) |
| charge (electrical, Coulombs) | மின்னூட்டம் | never கட்டணம் (billing) |
| ground plane (RF reference) | தரை-தளம் | not மண் (soil) |
| ground (GN/GD electrical) | தரை | |
| gain (antenna directivity, dB) | ஆதாயம் | never இலாபம் (profit) |
| wire (conductor) | கம்பி | not கேபிள் (cable/cord) |
| load (impedance) | சுமை | electrical loading sense |
| impedance | இம்பீடன்ஸ் (or மின்தடை where already established) | keep one choice consistent per string family |
| pattern (radiation) | வடிவம் / பிரிவகம் | 3D directional response, not "template" |
| excitation | உற்சாகம் ✗ — use தூண்டல் | EM energy input, not emotional excitement |
| segment (NEC2 geometry) | பிரிவு | |
| patch (NEC2 geometry) | தொகுதி | |
| tag (NEC2 identifier) | குறிச்சொல் | |
| frequency | அதிர்வெண் | |
| radial | ஆரக்கம்பி | ground-plane radial wire |
| file | கோப்பு | |
| window | சாளரம் | |
| settings | அமைவுகள் | |
| error | பிழை | |
| card (NEC2 data card, generic) | அட்டை | |
| thread | த்ரெட் | transliteration, standard CS usage |
| array | அணி | |
| shader | ஷேடர் | transliteration |
| color projection | நிற ப்ரொஜெக்ஷன் | transliteration, no established native term |
| polarization | முனைவு | |
| Gain Style (UI dropdown) | ஆதாய பாணி | |
| Noise Temperature | இரைச்சல் வெப்பநிலை | |
| sky model (noise temp) | வான் மாதிரி | |
| earth model (noise temp, planet) | பூமி மாதிரி | distinct from "ground" (தரை) RF sense |
| radiation pattern | கதிர்வீச்சு வடிவம் | |
| efficiency | திறன் | |
| solid angle | திண்ம கோணம் | |
| voltage | மின்னழுத்தம் | |
| interpolation | இடைச்செருகல் | |
| algorithm | வழிமுறை | |
| deadlock | டெட்லாக் | transliteration |
| child process | சேய் செயல்முறை | literary "சேய்" for child |
| notifier | அறிவிப்பாளர் | |
| token (parser) | டோக்கன் | transliteration |
| operand | ஆபரண்ட் | transliteration |
| operator (expression) | ஆபரேட்டர் | transliteration |
| arity | ஆரிட்டி | transliteration, niche term |
| resource (theme/UI asset) | வளம் | |
| symbol overrides (sy_expr) | குறியீடு மேலெழுதல்கள் | |
| near field | அண்மைப் புலம் | |
| E-field | மின்புலம் | |
| H-field | காந்தப்புலம் | |
| Poynting vector | பாய்ண்டிங் வெக்டர் | transliteration, proper noun |
| dielectric constant | டையிலெக்ட்ரிக் மாறிலி | transliteration + Tamil "constant" |
| batch mode | தொகுதி பயன்முறை | |
| treeview | மரக்காட்சி | |
| convergence (numerical) | ஒருங்கிணைவு | |
| expression (math/sy_expr) | கோவை | standard Tamil math term for an expression; not வெளிப்பாடு (manifestation/utterance sense) | |
| colon (punctuation) | முக்காற்புள்ளி | |

## Never Translate
- NEC2 mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN, GC, GM, GX, GR, GS, GN, GD, NE, NH,
  NT, TL, EK, KH, XQ, SY, SP, SC, SM, CM, CP, PQ, PT
- Units/symbols: MHz, dBi, Ω, VSWR, dB, S-parameters
- File extensions: .nec, .csv, .s1p, .s2p, .png, .gplot

## Disambiguation Policy
- Use correct technical sense per glossary above; do NOT append extra qualifying words
  (e.g. "மின்") beyond what the English source has, unless the sentence would otherwise
  be genuinely ambiguous in Tamil for a non-EE reader — the application context (EM
  simulator) normally disambiguates for the target audience of RF engineers.
- For niche EM/antenna nouns without a widely recognized native Tamil equivalent, prefer
  a transliteration of the English term over an invented neologism, for comprehension by
  working RF/antenna engineers.

## Format Specifiers
- Preserve %s, %d, %f, %c, %% in identical order to the source; restructure surrounding
  Tamil (SOV) sentence around fixed specifier positions.

## Numbers / Locale
- Decimal point (.) as in source, not comma.
