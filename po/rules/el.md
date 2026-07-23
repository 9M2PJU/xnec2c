# el (Greek) translation rules

## 1. Character set, symbology, expectations

- Modern monotonic Greek orthography (post-1982 reform): 24-letter alphabet
  Α-Ω/α-ω; single stress accent (tonos) on the stressed vowel of
  polysyllabic words (ά έ ή ί ό ύ ώ); diaeresis (ϊ ϋ) only when adjacent
  vowels must not form a diphthong. No polytonic diacritics (no breathing
  marks, no circumflex) anywhere in this catalog.
- Final-sigma rule: word-final lowercase sigma is always `ς`; medial/initial
  is `σ`.
- Mathematical/variable-name Greek letters used as symbols in tooltips
  (γ, φ, θ, Ω) are notation, not prose; leave them exactly as in the msgid
  (eg `γ = 0.5`, `φ=0`). Existing catalog precedent (lines 222, 227, 465,
  2022-2023) keeps `φ=0` verbatim inside otherwise-translated strings.
- Decimal separator: catalog precedent keeps the period in fixed numeric
  literals inside tooltips (`γ = 0.5`, not `γ = 0,5`), matching the
  period-decimal convention used by the program's own numeric entry
  widgets. Do not localize these literals to comma-decimal.
- Question mark: use ASCII `?`, matching numeric/technical UI convention
  already in the catalog (no Greek erotimatiko `;` in use).
- Never translate: NEC2 card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN),
  unit symbols (MHz, dBi, Ω, VSWR), file extensions (.nec, .csv, .s1p,
  .s2p, .png). NEC2 geometry loanwords `patch`/`Tag` stay in Latin script
  per existing catalog precedent (line 350 area: "Αριθμός Tag Φορτίου";
  "Patches" msgstr kept as "Patch").

## 2. Technical program-interface writing standard

- Contemporary Dimotiki (demotic) register throughout; no Katharevousa
  forms.
- Menu items, buttons, and short UI labels: noun-phrase form, not
  imperative verbs (eg "Άνοιγμα" not "Ανοίξτε"). This is the standing
  GNOME/GTK Greek desktop convention and matches this catalog's own
  precedent (eg "Παράμετροι Διέγερσης", "Ακτινωτό Πλέγμα Γείωσης").
- Dialog questions and instructions addressed directly to the user use
  full declarative/interrogative sentences in the formal plural (see
  §3), eg "Είστε βέβαιοι ότι θέλετε ...;".
- Status/log/error messages: declarative technical sentences using
  established Greek electrical-engineering vocabulary; keep loanwords
  for terms without a naturalized Greek equivalent.
- Compound technical nouns follow Greek genitive-chain order (eg
  "Σύνθετη αντίσταση εκτροπής" style), not literal English word order.

## 3. Formality mapping

- TRANSLATING.md mandates plural formal constructions for Greek in
  professional contexts; this catalog follows that: any verb directly
  addressing the user takes the formal plural (εσείς) conjugation, never
  the singular informal (εσύ).
- Because menu/button labels use noun-phrase form (§2), the formality
  question mostly does not arise there; it applies to full-sentence
  dialogs and confirmation prompts.
- Never use singular "είσαι"-class verb forms in this interface.

## 4. NEC2 electromagnetic-simulation domain mapping (established in this catalog)

| English (correct sense)        | Greek term            | Notes |
|---------------------------------|------------------------|-------|
| current (electrical)            | Ρεύμα / Ρεύματα        | never "τρέχον" (temporal) |
| charge (electrical, Coulombs)   | Φορτίο / Φορτία        | domain-disambiguated, no added qualifier |
| load (impedance)                | Φορτίο                 | same noun as charge; NEC2/LD-card context disambiguates |
| ground / ground plane           | Γείωση                 | never "έδαφος" (soil); electrical ground-plane sense (GN/GD params, ground conductivity/type, perfect ground) |
| ground wave (propagation)       | κύμα εδάφους           | distinct radio-propagation sense; keep εδάφους, not γείωση |
| below ground (geometry z<0)     | κάτω από το έδαφος     | spatial earth-surface sense; γείωση would misread as "below the grounding" |
| wire / conductor                | Αγωγός / Αγωγών        | not "σύρμα" (generic cord) |
| gain (antenna directivity, dB)  | Κέρδος                 | |
| excitation                      | Διέγερση                | |
| inductance                      | Επαγωγή                 | |
| impedance                       | Εμπέδηση                | catalog precedent: "Impedance (real/imag)" -> "Εμπέδηση (πραγμ./φανταστ.)" |
| pattern (radiation)             | Διάγραμμα (ακτινοβολίας)| |
| patch / Tag (NEC2 geometry term)| Patch / Tag (unchanged)| Latin loanword, catalog precedent |
| polarity (sign, +/-)            | Πολικότητα              | false friend of "polarization"; keep distinct from Πόλωση |
| polarization (antenna axis)     | Πόλωση                  | never used for "polarity" (sign) sense above |
| animated / animation            | Κίνηση                  | menu-header/category label, no mnemonic underscore unless msgid carries one |
| static (phase-invariant read)   | Στατικό                 | catalog precedent: "Ροή Patch — Στατικό" |
| comet (moving-crest overlay)    | Κομήτης                 | animation overlay name, not "Γεωμετρία" |
| overlay(s)                      | Επικάλυψη / Επικαλύψεις | UI overlay toggles on the animation dialog |
| hue/brightness encoding         | κωδικοποίηση απόχρωσης / φωτεινότητας | internal color-pipeline error strings |
| color tone (transfer curve)     | τόνος χρώματος          | distinct from "scale family" (Οικογένεια κλίμακας) |
| palette kind                    | τύπος παλέτας           | distinct from "color tone" above |
| power (power-law tone curve n^γ)| Δύναμη                  | mathematical exponentiation, never Ισχύς (electrical watts); label, "power transfer", and "using power" fallback all take δύναμη, matching "Μετασχηματισμός δύναμης n^γ". Reserve Ισχύς for physical power (radiated/dissipated power, power gain, power-flow). |
| power (electrical, watts)       | Ισχύς                   | radiated/dissipated power, power gain (κέρδος ισχύος), Poynting power-flow (ροή ισχύος) |
| knee (compressor soft knee k)   | Γόνατο                  | UI slider label is "Γόνατο:"; use everywhere for the knee parameter k, never "σημείο καμπής" |

No qualifier such as "ηλεκτρικό/ηλεκτρικά" is added to Ρεύμα/Φορτίο where
the electromagnetic-simulator program context already disambiguates the
sense (matches TRANSLATING.md's Afrikaans/German/Spanish examples).

## 5. Fuzzy-repair caution

Automatic fuzzy-matching in this catalog has previously carried a msgstr
across to an unrelated msgid with similar English wording (eg "Polarity"
inheriting "_Πόλωση" from "_Polarization"; "Comet" inheriting "Γεωμετρία"
from "Geometry"; several `%d`-only chroma/palette error strings all
inheriting the same wrong sentence). When repairing a fuzzy entry, always
re-derive the translation from the current msgid text and verify against
its own tooltip/context rather than trusting the inherited msgstr.
