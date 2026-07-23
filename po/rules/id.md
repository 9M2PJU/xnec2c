# id translation rules

## 1. Character set, symbology, expectations

- Script: Latin, standard 26-letter alphabet, no diacritics required for Bahasa Indonesia baku.
- Number formatting is left to libc/locale rendering, not to translator-authored text; do not rewrite decimal points inside format-specifier strings.
- Unit symbols and NEC2 mnemonics are Latin already, so no transliteration issue: keep `dBi`, `MHz`, `Ω`, `VSWR`, `GW`, `EX`, `LD`, `FR`, `RP`, `GE`, `EN` unchanged.
- No case distinction rules like German noun-capitalization; Indonesian is written lower-case except sentence-initial and proper nouns. Menu/button title-case from English is not carried over; use normal sentence case unless the catalog already establishes a convention for a given string family.

## 2. Technical interface writing standards

- Use Bahasa Indonesia baku (standard, formal register) per EYD (Ejaan Yang Disempurnakan) spelling; avoid slang, regionalisms, and unnecessary English calques.
- Direct imperative verbs for menu items and buttons, matching source imperative mood: "Save" → "Simpan", "Open" → "Buka", "Exit" → "Keluar".
- Established computing loanwords already used throughout this catalog are kept for consistency: `file` (NOT `berkas`), `klik`, `thread`, `byte`, `debug`, `verbose`, `parameter`, `frekuensi`. Do not introduce `berkas` where `file` already dominates the catalog.
- Compounds are written as separate words with prepositions/genitive order reversed from English, e.g. "current distribution" → "distribusi arus" (noun + qualifier, not qualifier + noun as in German).

## 3. Formality and informality

- Use formal "Anda" (capitalized) for second person in any user-facing dialog text; never use informal "kamu"/"lu".
- UI imperative commands (buttons/menu items) omit both "Anda" and softening particles like "silakan"; keep them terse, matching English UI convention (e.g. plain "Simpan", not "Silakan simpan"). "silakan"/"SILAKAN" is permitted only inside dialog/error prose that mirrors an English "please".
- Add a GTK mnemonic underscore only where the source msgid carries one at the corresponding letter; never invent a mnemonic for a label whose msgid has none (e.g. radio label "Animated" → "Animasi", but menu "A_nimate" → "A_nimasi"). Sibling controls in one group share the source's presence/absence of mnemonics.
- Menu-path references inside message text must name the menu by its translated label; the File menu is "_File", so cross-references read "File->…", never "Berkas->…".
- Dialog/confirmation messages addressing the user directly use "Anda" where the English source has "you" (e.g. "Are you sure..." → "Apakah Anda yakin...").

## 4. NEC2 electromagnetic-simulation domain mapping

Established terminology already present and reused in this catalog (Indonesian amateur-radio/EE convention, consistent with ORARI usage):

| English | Indonesian (established) |
|---|---|
| current (electrical) | arus |
| charge (electrical) | muatan |
| gain | penguatan (verb/process) / gain (noun, kept when paired with dBi) |
| load / impedance-loading | beban / pembebanan |
| impedance | impedansi |
| excitation | eksitasi |
| radiation pattern | pola radiasi |
| polarization | polarisasi |
| ground plane | ground plane (kept, RF jargon) |
| segment / patch / tag | segmen / patch / tag (NEC2 geometry terms, unchanged) |
| wire | kawat |
| frequency | frekuensi |
| file | file (not berkas, per existing catalog convention) |
| viewer (3D view/observer direction) | Viewer (kept untranslated, established catalog term; never "Pengamat") |
| None (UI option) | Tidak Ada (title-case, matching catalog convention incl. "Pilih Semua/Tidak Ada"); parenthetical qualifier stays lower-case, e.g. "Tidak Ada (geometri)" |

Never translate: NEC2 card mnemonics, VSWR, dBi, S-parameter, Ω, MHz, file extensions (.nec, .csv, .s1p, .s2p, .png).

Disambiguation follows doc/TRANSLATING.md: pick the electrical/EM sense silently from program context; do not append explanatory qualifiers (e.g. "Lihat Arus", not "Lihat Arus Listrik").
