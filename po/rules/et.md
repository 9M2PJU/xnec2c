# Estonian (et) translation rules

## 1. Character set and symbology

- Latin script with Estonian-specific letters: õ, ä, ö, ü, š, ž (also Š, Ž). Type them directly (UTF-8), never transliterate to õ→o, ä→a, ö→o, ü→u.
- Decimal separator in Estonian prose is a comma (`50,0`), but runtime-formatted numbers (`%f`, `%d` etc.) are locale-formatted by the C library, not by the msgstr text itself; leave format specifiers untouched (see Format specifiers below).
- Quotation marks: plain ASCII `'...'`/`"..."` are acceptable and already used throughout the catalog (matches existing entries); do not switch to „…" typographic quotes mid-catalog.
- Units/mnemonics stay Latin/ASCII and untranslated: VSWR, dBi, MHz, Ω, NEC2, GW/GA/GH/EX/LD/FR/RP/GE/EN, file extensions (.nec, .csv, .s1p, .s2p, .png).

## 2. Technical interface writing standards

- Estonian capitalizes only the first word of a sentence/title and proper nouns; do NOT title-case every word (unlike German/English menu conventions). "Color Scale" → "Värviskaala", not "Värvi Skaala".
- Compound technical terms are formed as single compound words or genitive-noun chains per standard Estonian orthography (e.g. "voolujaotus", "kiirgusdiagramm"), not multi-word calques of English structure.
- Commands/buttons/menu items use the 2nd person singular imperative (käskiv kõneviis), independent of formality register: "Salvesta" (Save), "Ava" (Open), "Sule" (Close), "Lähtesta" (Reset).
- Labels ending in ':' in the source (e.g. "Gamma:") keep the trailing colon in Estonian: "Gamma:".

## 3. Formality mapping

- Direct questions/confirmations addressed to the user use the formal "teie" verb form (2nd person plural used as polite singular), matching existing catalog usage, e.g. "Kas olete kindel..." / "Kas soovite...". Do not use "sina"-form (soovid, oled) anywhere in the UI.
- Commands/buttons remain imperative (see §2) and carry no formality marking themselves; this is distinct from the "teie" dialogs described here.
- Informative/status messages are impersonal, verb-first or noun-phrase constructions ("Salvestamine ebaõnnestus", "Faili ei leitud") — no pronoun needed; this differs from both §1 (script) and §4 (domain lexicon).

## 4. NEC2 electromagnetic-simulator domain lexicon (Estonian)

Established/preferred terms — use these consistently across the catalog:

| English | Estonian | Notes |
|---|---|---|
| current (electrical) | vool | never "praegune" (temporal) |
| charge (electrical) | laeng | never "tasu" (billing) |
| voltage | pinge | |
| impedance / load | takistus / impedants | "takistus" for load context, "impedants" when explicitly about complex impedance |
| ground / ground plane | maandus / maandustasand | never "muld" (soil) |
| wire | traat | never "kaabel" (cable) |
| gain | võimendus (RF gain) | antenna directivity ratio, not "kasum" (profit) |
| radiation pattern | kiirgusdiagramm | |
| excitation | ergastus | already established: "Ergastuse tüüp" |
| segment / patch / tag | segment / plaat (pinnaplaat) / silt | keep NEC2 term "segment" as loanword; "patch" → "(pinna)plaat" |
| frequency | sagedus | |
| phase | faas | |
| magnitude / amplitude | amplituud / tugevus | "tugevus" for current/charge magnitude along wires (incl. "Instantaneous Magnitude" → "Hetkeline tugevus"), "amplituud" for peak value contexts (incl. "Peak magnitude" → "Tipu amplituud") and |Z| magnitude |
| flow direction | voolusuund | established; direction terms compound genitive-stem + "suund" |
| gain direction | võimendusesuund | compound like "voolusuund"; use consistently ("Max Gain Direction" → "Maks võimendusesuund") |
| wireframe | traatraam | established |
| patches (surface) | pinnaplaadid / pind | established |
| color projection | värviprojektsioon | |
| color scale / family | värviskaala / värvipere | |
| thread (SMP) | lõim | established: "lõimede arv" |
| geometry | geomeetria | |
| validation | valideerimine | |
| configuration | seadistus / konfiguratsioon | "seadistus" preferred for UI settings, "konfiguratsioon" for config files/CLI |
| feedpoint | toitepunkt | |
| port | port | loanword, keep as-is |
| polarity (sign, +/−) | polaarsus | never "polarisatsioon" (antenna wave polarization); false-friend risk, keep the two senses distinct |
| polarization (antenna wave) | polarisatsioon | eg "Polarization axis" → "Polarisatsiooni telg"; never used for sign polarity |
| node / antinode (standing wave) | sõlm / kõhk | "sõlmed/kõhud" for the plural UI pair; never "antisõlm" (calque) |
| hue encoding / brightness encoding (internal enum) | toonikodeering / heleduskodeering | internal chroma-source encoding selectors; distinct from the user-facing "värviprojektsioon" (Color projection) label |
| palette kind (internal enum) | paletiliik | distinct from "skaala pere" (scale/tone family, §-linked to "Scale family:") |
| ramp / gradient (palette kind, color scale) | gradient | loanword; one term for both "ramp" and "gradient" senses, eg "värvigradient" (Color gradient), "Vikerkaar-gradient" (Rainbow ramp) |
| floor (minimum/lower clamp of a value) | alammäär | never "põrand" (room floor, calque); use one stem for the label ("Brightness floor" → "Heleduse alammäär") and every "floor" tooltip (floored → "alammääraga", below the floor → "alammäärast madalam", hard floor → "range alammäär") |
| bins (discretization buckets, eg depth sorting) | vahemikud | never "täkked" (notches/tallies); "Depth Bins" → "Sügavuse vahemikud" |
| override (supersede a value, eg SY symbol) | alistus | never "ülekirjutus" (overwrite); "Symbol Overrides" → "Sümbolite alistused" (plural title/menu); after a count use partitive singular "%d sümboli alistust" |
| overlay (added visual layer) | kiht | eg "Overlays:" → "Kihid:", "This overlay" → "See kiht"; keep consistent with layer sense |

## 5. Format specifiers

Preserve every `%s`, `%d`, `%f`, `%c`, `%%`, `%2$s` etc. from msgid in the msgstr, same set and order (or explicit positional `%N$s` if Estonian word order requires reordering). Estonian syntax rarely requires reordering since it is fairly free word-order but SVO-default; keep source order by default.
