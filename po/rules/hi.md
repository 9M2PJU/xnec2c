# hi (Hindi) translation rules

## 1. Native character set, symbology, expectations

- Script: Devanagari (देवनागरी), Unicode block U+0900–U+097F. Conjuncts and
  matras render via standard Unicode composition; no manual ligature work
  needed.
- Numerals: use Western/Arabic digits (0-9) for all technical values,
  counts, frequencies, and units (`MHz`, `dBi`, `Ω`, `%`) — never Devanagari
  numerals (०-९). This matches existing entries (`आवृत्ति MHz`, `300ms`,
  `S2P`, etc.) and Indian technical-software convention (Android/Windows
  Hindi locales).
- Punctuation: the Devanagari danda (।) is the sentence-final full stop and
  is used throughout this catalog (~177 occurrences); keep it for full stops.
  Use Western punctuation for inline separators (`,` `:` `;` `!` `?` `"…"`)
  and for abbreviations, decimals, units, and Latin technical tokens. Never
  substitute a Latin period `.` for a sentence-final danda, and never mix the
  two terminators across sibling strings.
- No case distinction exists in Devanagari (no uppercase concept); casing
  rules from Latin-script languages do not apply.
- NEC2 mnemonics, file extensions, unit symbols, and other Latin-script
  technical tokens stay embedded in Latin script inside Devanagari
  sentences (e.g. `NEC2 सिम्युलेटर`, `Segment संख्या`, `आवृत्ति MHz`).

## 2. Technical interface writing standards

- Follow the established register already in this catalog: standard
  modern technical/textbook Hindi (as used in NCERT physics/engineering
  textbooks and mainstream Indian tech UI), not heavily Sanskritized
  "shuddh" Hindi and not colloquial Hinglish slang.
- Common English technical nouns that lack a natural concise Hindi
  equivalent stay transliterated in Devanagari rather than invented as
  Sanskrit neologisms: `फ़ाइल` (file), `मेमोरी` (memory), `थ्रेड` (thread),
  `सिम्युलेटर` (simulator), `स्केल` (scale), `टेक्सचर` (texture).
- Physics/EE quantities that have a well-established Hindi term from
  standard curricula are translated, not transliterated: `धारा` (current),
  `आवेश` (charge), `प्रतिबाधा` (impedance), `आवृत्ति` (frequency), `लाभ`
  (gain), `उत्तेजना`/`उत्तेजन` (excitation), `कोण` (angle), `चालकता`
  (conductivity).
- Devanagari does not concatenate compounds like German; render multi-word
  technical phrases as spaced Hindi phrases (`ट्रांसमिशन लाइन प्रतिबाधा`),
  not fused single words.
- SOV (subject–object–verb / verb-final) sentence order; when a source
  sentence contains a format specifier, keep the specifier in source order
  and restructure the surrounding clause around it (see doc/TRANSLATING.md
  Format Specifiers section), reordering with positional args (`%2$s`) only
  when grammar requires it.
- Imperative UI actions (buttons, menu items) use formal command verb
  forms (`चुनें`, `सक्षम करें`, `रीसेट करें`), not casual imperative stems.
- GTK mnemonic underscores belong only to a msgstr whose msgid carries a `_`;
  never import a `_` into a dialog title or message that has none in the
  source. The dialog titles "Batch Math Library", "Mathlib Help", "Mathlib
  Benchmark", and "Mathlib Benchmark Help" take no `_`, even though the
  mirroring menu items ("Mathlib _Help", "Mathlib _Benchmarks") do.
- Preserve the source case of a coordinate variable inside a diagnostic
  string: lowercase `z` in "step size limited at z=" / "Hankel not valid for
  z = 0" stays lowercase `z`, not `Z`. The X/Y/Z uppercase axis-label
  convention (§1) applies to axis names, not to a lowercase math variable
  echoed from the C source.

## 3. Formality / informality

- Use आप (formal "you") and its verb agreements throughout; never तू/तुम.
- Formal imperative endings: `-करें` (do-formal), `चुनें` (choose-formal),
  `देखें` (view-formal) — not `-करो`/`-देखो` casual forms.
- This axis is independent of script mechanics (§1) and lexical choice
  between transliteration vs. translation (§2): formality governs verb
  conjugation and pronoun choice only.

## 4. NEC2 electromagnetic-simulator domain mapping

- Never translate: NEC2 identifiers, card mnemonics (`GW`, `GA`, `GH`,
  `EX`, `LD`, `FR`, `RP`, `GE`, `EN`), `segment`/`patch`/`tag` as bare
  geometry-card field labels when paired with the mnemonic (eg `Segment
  संख्या`), `VSWR`, `dBi`, `S-parameters`, file extensions
  (`.nec`, `.csv`, `.s1p`, `.s2p`, `.png`), unit symbols (`MHz`, `dBi`,
  `Ω`, `dB`, `degrees`).
- Established glossary (must stay consistent everywhere):
  - current → धारा
  - charge → आवेश
  - impedance → प्रतिबाधा
  - excitation → उत्तेजन (used as both noun and adjective throughout, eg
    "उत्तेजन प्रकार", "उत्तेजन कमांड", "उत्तेजन-पोर्ट")
  - frequency → आवृत्ति
  - gain → लाभ
  - ground plane → ग्राउंड प्लेन (transliterate; RF-specific sense, no
    natural concise Hindi equivalent in this domain)
  - radiation pattern → विकिरण प्रतिरूप / लाभ पैटर्न (per existing usage)
  - wire → तार
  - load → लोड (RF impedance sense; transliterate to avoid collision with
    common Hindi words for physical weight)
  - port → पोर्ट
  - feedpoint → फ़ीडपॉइंट
  - scale → स्केल
  - peak → शिखर
  - phase → फ़ेज
  - element (geometry-card dialog heading — Arc/Helical/Patch/Wire Element)
    → तत्व, consistently; never एलिमेंट/एलीमेंट/अवयव. A matrix entry is a
    distinct sense ("Element 1,1", "Admittance Matrix Elements") → अवयव.
  - normalize / normalized / normalization → सामान्यीकरण / सामान्यीकृत
    (translate; never transliterate नॉर्मलाइज़ / नॉर्मलाइज़्ड / नॉर्मलाइज़ेशन).
  - minor (axis) → लघु; major (axis) → मुख्य (never माइनर).
  - card → कार्ड in running Hindi prose (eg "NE कार्ड जोड़ें", "कार्ड हटाएं");
    keep the source English "Card"/"card" inside a mnemonic-paired dialog-title
    parenthetical ("(GA card)", "(RP Card)"), matching the source case.
  - wavelength → तरंगदैर्घ्य (from दीर्घ → दैर्घ्य; always spell with घ्य,
    never the misspelling तरंगदैर्ध्य)
  - patch → **never translate** (bare NEC2 geometry term alongside
    segment/tag, per doc/TRANSLATING.md); keep the English word "patch"/
    "Patch" untranslated at every site, matching its case in the source
    (eg "Patch" for the capitalized heading, "patch" mid-sentence).
  - surface (only in the fixed compound "surface patch") → सरफेस patch;
    "surface" alone (not part of that compound) still translates fully,
    eg "Surface" → सतह, "Surface Subdivision" → सतह उप-विभाजन.
- Disambiguation: select the electrical/EM sense per doc/TRANSLATING.md;
  do not append a qualifier word (eg "विद्युत") that is absent from the
  English source — the simulator context already disambiguates for the
  domain-expert reader.
