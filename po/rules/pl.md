# pl translation rules

## 1. Native character set, symbology, expectations

- Latin script with Polish diacritics: ą ć ę ł ń ó ś ź ż (and Ą Ć Ę Ł Ń Ó Ś Ź Ż). Use them; never substitute plain ASCII look-alikes (eg `zysk` not `zysk`, but `chcesz` never `chcesz` — diacritics are mandatory, not decorative).
- Straight double quotes `"..."` are standard in this catalog's existing entries (not the typographic „...” pair); follow existing practice for consistency rather than switching styles mid-catalog.
- Unit symbols and NEC2 mnemonics stay unchanged: MHz, dBi, Ω, %, °, GW/EX/LD/FR/RP/GE/EN, VSWR, file extensions (.nec, .csv, .s1p, .s2p, .png).
- Ellipsis: use `...` (three periods), matching existing catalog usage.
- No case-folding issues; Polish has no letter-casing ambiguity like Turkish dotted/dotless i.

## 2. Technical interface writing standards

- Menu items, buttons, labels: short noun phrases or imperative verb forms (eg "Zapisz" = Save, "Anuluj" = Cancel), sentence case, not Title Case.
- Error/status messages: lead with the domain noun, eg "Błąd ..." (Error ...), "Nie można ..." (Cannot ...) — matches established catalog usage.
- Confirmation dialogs use the established idiom "Czy na pewno chcesz ...?" (Are you sure you want to ...?) — already the catalog convention; keep it for new confirmation strings.
- Keep sentences compact; Polish technical prose favors terse noun chains over English gerunds (eg "Skalowanie zysku" not "Skalowanie dla zysku").

## 3. Formality mapping

- Polish software UI conventionally addresses the user with informal second-person singular verb forms without a pronoun (eg "chcesz", not formal "Pan/Pani chce" or impersonal passive). This is the dominant convention in Polish open-source software (GNOME, KDE) and is already used in this catalog ("Czy na pewno chcesz zamknąć xnec2c?").
- Do not introduce formal Pan/Pani address; it would be inconsistent with the rest of the catalog and atypical for this class of engineering tool.
- Sections 1-3 are disjoint: §1 is script/symbols, §2 is sentence-level phrasing shape, §3 is person/register of address — no overlap.

## 4. NEC2 domain mapping for Polish

| English | Polish | Notes |
|---|---|---|
| current(s) | prąd / prądy | electrical current, established in catalog |
| charge(s) | ładunek / ładunki | electrical charge |
| gain | zysk | antenna directivity, established |
| ground / ground plane | ziemia / płaszczyzna ziemi | NEC2 earth reference; unify on "ziemia" root everywhere, never "masa" (chassis-ground sense) nor "uziemienie" (act of earthing) |
| excitation | wzbudzenie | established |
| impedance | impedancja | established |
| wire | drut | thin conductor, established |
| load | obciążenie | electrical impedance/load, not physical weight |
| pattern (radiation) | charakterystyka (promieniowania) | established |
| segment | segment | NEC2 geometry term, unchanged |
| patch | płat / płatek | surface patch, established as "płat" |
| feedpoint | punkt zasilania | established |
| color projection | projekcja kolorów | new UI feature (color-family rendering) |
| color family / scale family | rodzina koloru / rodzina skali | new UI feature |
| color tone (transfer function family: Power/Asinh/μ-law/Reinhard/Sigmoid) | tryb tonalny | distinct from "rodzina koloru" (palette family) and "projekcja koloru" (hue/brightness projection); do not conflate |
| hue | odcień | |
| hue encoding / brightness encoding | kodowanie odcienia / kodowanie jasności | internal chroma dispatch error strings |
| gamma | gamma | keep as-is, math term |
| dynamic range | zakres dynamiki | |
| softening knee | próg (miękkiego) załamania | soft-compression control |
| compression | kompresja | |
| contrast | kontrast | |
| brightness | jasność | |
| magnitude | amplituda | peak/instantaneous signal magnitude s(n) |
| standing wave / traveling wave | fala stojąca / fala bieżąca | established RF pair |
| node / antinode | węzeł / strzałka (fali) | standing-wave nulls/peaks; "strzałka" is the standard Polish physics term for antinode, never "przeciwwęzeł" |
| comet (animation overlay) | kometa | animation overlay name, not a geometry term |
| far-field contribution (projection) | wkład w pole dalekie | do not confuse with "Near Field Animation" (Animacja pola bliskiego) — distinct features |

Do not add qualifiers (eg "elektryczny/elektryczna") absent from the English source for current/charge/gain/load/ground; the electromagnetic-simulator context already disambiguates for the target audience.

## Hotkey policy

- GTK mnemonics use `_` before the access letter. Existing catalog already assigns Polish-specific hotkeys distinct from English originals (eg `_Wizualizacja`, `Zysk _netto`). When translating a fuzzy/untranslated string with a `_`-marked hotkey, pick a letter unique within its menu/dialog, preferring the letter used by an established analogous string.
