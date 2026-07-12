# bs translation rules

## Script and character set
- Latin script (Gajica) with mandatory diacritics: č, ć, š, ž, đ, dž, lj, nj.
- Do not ASCII-fold diacritics (e.g. "č" must not become "c").
- Unit symbols (MHz, dBi, Ω, VSWR) and NEC2 mnemonics (GW, EX, LD, FR, RP, GE, EN) stay unchanged.
- File extensions (.nec, .csv, .s1p, .s2p, .png) stay unchanged.

## Register
- Formal imperative without explicit pronoun for actions/commands: "Odaberite...", "Prikaži...", "Animirajte...".
- Noun phrases for labels/menu items: "Boja", "Skaliranje", "Faza".
- No T-V pronoun distinction required; imperative form already implies formal register.

## Established lexicon (reuse for consistency)
| English | Bosnian |
|---|---|
| Currents | Struje |
| Charges | Naboji |
| Visualization | Vizualizacija |
| Scale | Skaliraj / Skaliranje |
| Peak value | Vršna vrijednost |
| Peak magnitude | Vršna amplituda |
| Instantaneous | Trenutačno |
| Phase | Faza |
| Polarization axis | Os polarizacije |
| Axis (X/Y/Z, major/minor) | os (i-declension only: N os, G/L osi; never a-declension osa/ose) |
| Wireframe | Žičani okvir |
| Reference phase | Referentna faza |
| Flow direction | Smjer toka |
| Patch (surface patch) | Zakrpa |
| default | podrazumijevano |
| Color projection | Projekcija boje |
| Color family / Color Scale | Porodica boja / Skala boja |
| widget | widget (untranslated) |
| geometry | geometrija |
| Excitation | Pobuda |
| Excitation type | Tip pobude |
| Gain (antenna) | Dobitak (masc.; agrees: ukupni dobitak, neto dobitak, sirovi/maksimalni dobitak, dobitak posmatrača) |
| feedpoint | napojna tačka |
| Validation | Verifikacija/validacija (validation tree = stablo validacije) |

## Domain disambiguation (never over-qualify beyond source)
- current → struja (electrical), never "trenutno" (temporal)
- charge → naboj (electrical), never "naplata" (billing)
- ground → uzemljenje (NEC2 GN/GD earth ground: "Ground present/plane/type/conductivity"); one term catalog-wide, never "tlo" for the GN/GD ground. "masa" only for a true circuit-reference ground (none in NEC2).
- earth → tlo / zemlja when the source word is literally "earth" (e.g. earth-model index, sky/earth noise-temperature models); this physical-terrain sense stays distinct from the GN/GD "ground".
- gain → dobitak (masculine noun; antenna directivity dB), never "pojačanje" (amplifier gain) and never "dobit/profit" (financial). Adjectives agree in masculine: ukupni dobitak, neto dobitak, sirovi dobitak, maksimalni dobitak.
- pattern → dijagram (radiation pattern), never "šablon" (template)
- load → opterećenje (electrical impedance), never generic "teret" (physical weight)
- Match source qualification level; do not add "električni/električna" unless the English source itself is ambiguous without it.

## Format specifiers
- Preserve %s, %d, %f, %c, %%, %llu, %lu, %g in the same order as msgid; Bosnian word order rarely requires reordering, but where needed use positional %1$s syntax.
