# sk translation rules

## Script and typography
- Full diacritics required at all times: á ä č ď é í ĺ ľ ň ó ô ŕ š ť ú ý ž.
- Decimal separator is comma in locale-formatted numbers (`50,0`), independent of C-format `%f`/`%d` specifiers which are never altered.
- Quotes: use plain `'...'` for inline quoted widget/identifier names, matching existing catalog usage; do not introduce „..." typographic quotes into format strings.
- Units/mnemonics never translated: MHz, dBi, Ω, VSWR, S-parameters, GW/GA/GH/EX/LD/FR/RP/GE/EN, file extensions (.nec, .csv, .s1p, .s2p, .png).

## Register and structure
- Menu items / button labels: short noun phrases or polite imperative ("Vyberte...", "Zapísať...").
- Field labels: noun + colon, e.g. "Mierka:", "Kontrast:", "Uhol:".
- Tooltips / help text: polite formal imperative ("Vyberte spôsob..."), 2nd person formal (Vy, implied not stated) register — never familiar/2nd-person-singular imperative.
- Error/log messages (pr_* strings, debug/internal): terse, impersonal, infinitive or noun-phrase style, consistent with existing entries like "Ignorujem CM kartu v geometrii".
- No compounding of unrelated technical nouns; keep as separate qualified noun phrases (Slovak does not freely compound like German).

## Formality
- Formal register throughout (implied "Vy"); never informal "ty" forms.

## Domain lexicon (locked, one term per meaning, use everywhere)
| English | Slovak | Notes |
|---|---|---|
| current (electrical) | prúd | never temporal "aktuálny" |
| charge (electrical) | náboj | never cargo/load sense |
| wire | drôt | |
| ground / ground plane | zem / uzemnenie / ground plane (RF-specific, may keep "ground plane") | electrical sense only |
| gain | zisk | antenna directivity, never "profit" |
| excitation | budenie / excitácia | keep "excitácia" when paired with card/port context already using it |
| pattern (radiation) | vyžarovací diagram | |
| load (impedance) | záťaž | electrical impedance sense |
| segment / patch / tag | segment / plôška (patch) / tag | NEC2 geometry terms, patch = "plôška" (surface patch context) |
| impedance | impedancia | |
| phase | fáza | |
| polarization | polarizácia | |
| scale | mierka | |
| color projection | farebná projekcia | |
| flow (current/patch flow) | tok (prúdu) | |
| feedpoint | napájací bod | |
| port | port | |
| threshold/knee | koleno / prah | "koleno" for soft-knee compressor context |
| brightness | jas | |
| contrast | kontrast | |
| compression | kompresia | |
| dynamic range | dynamický rozsah | |
| gamma (exponent) | gama | keep as γ symbol where source uses γ |
| wireframe | drôtený model | never "drôtový model"; locked spelling |
| geometry | geometria | |
| magnitude | veľkosť | field/current modulus; never "amplitúda" (reserved for amplitude) |
| amplitude | amplitúda | reserved for source word "amplitude" only |
| peak (signal value, adj.) | špičkový / špičková | "Peak value", "Peak magnitude", "peak-envelope" |
| peak (of curve/step, noun) | vrchol | maximum data point of a step/curve |
| validation | validácia / overenie | "validácia" for the Validation Tree feature name, "overenie" for verification-checks sense |
| widget | widget | loanword, established in catalog |
| card (NEC2 input line) | karta | |
| near field | blízke pole | |
| default | predvolený | |
| batch mode | dávkový režim | |
| thread | vlákno | |
| managed allocator | spravovaný alokátor | |

## Disambiguation
- Domain context alone disambiguates "prúd"/"náboj"; do not add "elektrický" qualifier unless source text itself qualifies.

## Format specifiers
- Preserve `%s %d %f %c %%` verbatim, same order as msgid; reorder only via `%2$s` positional syntax if Slovak grammar requires reordering.

## Hotkeys
- GTK `_` mnemonic precedes the access-key letter; when two labels in the same menu collide, shift the underscore to a different, keyboard-typable Slovak letter (avoid diacritic letters as mnemonics where an ASCII alternative exists in the same word).
- A diacritic mnemonic (Š, Č, Ž, Ľ, ...) is acceptable when it does not collide with a sibling; only shift it on collision.
- Radiation-pattern menu (rdpattern): `Š`-initial siblings must not all take `Š` — Škálovanie _zisku (z), Šumová _teplota (t), Štýl k_reslenia (r); Spoločná pro_jekcia (j) to avoid `p` clash with _Prekryť štruktúru.
- Visualization menu: Spoločná pro_jekcia (j) avoids `p` clash with Os _polarizácie; Špičková v_eľkosť (e).
