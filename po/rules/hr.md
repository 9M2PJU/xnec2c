# hr translation rules

## 1. Character set, symbology, expectations

- Latin script with Croatian diacritics: č, ć, đ, š, ž (lower) / Č, Ć, Đ, Š, Ž (upper). No ligatures.
- Alphabet order (for any future sorting): a b c č ć d đ e f g h i j k l m n o p r s š t u v z ž.
- No German-style eszett, no accented vowels beyond the five diacritics above.
- Quotation marks: catalog is plain ASCII UI text; keep straight `"..."` as used throughout, do not switch to „..." typographic quotes inside msgstr (source strings use plain quotes, e.g. widget names in quotes).
- Decimal point: numeric *format templates* in this catalog (e.g. `"0.0000"`, `"0.00000"`) are literal display masks tied to source code, not prose numbers — keep the period, do not localize to comma. Croatian prose decimal comma is not applicable here since no free-standing decimal numbers appear in translatable prose.
- Matrix/index pairs like "Element 1,1" use comma as an index separator (not decimal) — keep as-is, this is not a locale numeral.

## 2. Technical interface writing standards

- Sentence case for labels/menu items, matching source; do not title-case every word (Croatian does not capitalize each word).
- Menu/button imperatives use short 2nd-person verb forms already established in this catalog: `Zatvori`, `Primijeni`, `Odustani`, `Poništi na _zadane vrijednosti` — keep this established short-imperative style for new buttons/menu entries.
- Dialog/warning prose uses the formal register (see section 3) already present in the catalog: `Jeste li sigurni...`, `Molimo...`, `možete...`.
- Compound technical nouns follow existing catalog patterns, e.g. `boja` (color), `projekcija` (projection), `skala` (scale), `iznos` (magnitude), `faza` (phase) — reuse these established terms rather than introducing synonyms.

## 3. Formality and informality

- The catalog's established register is formal 2nd-person plural (Vi-form verbs, uncapitalized "vi"): `Jeste li sigurni`, `Molimo pročitajte`, `možete koristiti`, `Molimo ponovno odaberite`. New dialog/informational prose must match this formal register.
- Short button/menu imperatives (`Zatvori`, `Primijeni`, `Odustani`, `Poništi`) are the catalog's established neutral command form for controls — these are conventional in Croatian technical UIs and are not informal-register violations; keep using them for new buttons rather than switching to verbose formal-plural forms.
- Sections 1-3 do not overlap: 1 is script/symbol mechanics, 2 is structural/lexical convention, 3 is verb-register/address choice.

## 4. NEC2 electromagnetic-simulator domain mapping

- Never translate: NEC2, card mnemonics (GW, GA, GH, EX, LD, FR, RP, GE, EN); segment/patch/tag as geometry terms may translate (segment→"segment", patch→"zakrpa" as already used, tag→"tag" as already used) but the mnemonics themselves stay Latin.
- Keep unit symbols and technical abbreviations unchanged: MHz, dBi, Ω, VSWR, S-parametri (S-parameters may take Croatian plural suffix on "parametri" but keep "S-" prefix), dB.
- Disambiguate per domain sense without adding qualifiers absent from source (per doc/TRANSLATING.md policy): "struja" for current (electrical), "naboj" for charge (electrical), "uzemljenje"/"masa" for ground, "žica" for wire, "dobitak" for gain, "opterećenje"/"impedancija" for load depending on context, "zakrpa" for patch, "uzorak zračenja" for radiation pattern.
- Established catalog terms to reuse consistently: `struja` (current), `naboj` (charge), `faza` (phase), `iznos`/`veličina` (magnitude), `projekcija` (projection), `zakrpa` (patch), `žica` (wire), `pobuda` (excitation, as already used: `Vrsta pobude`).
- Computing/CLI lexicon (standardized here for whole-catalog consistency; the loanword/transliteration variants were corrected to the Croatian-normative forms and must not be reintroduced):
	- library → `knjižnica` (programska knjižnica), never `biblioteka`.
	- thread → `dretva`, never `nit`/`niti` in the thread sense (beware `niti` is also the conjunction "neither/nor" — leave those).
	- fork (process) → `račvanje`/`račvati`, never `forkanje`.
	- batch mode → `skupni način` (matches UI `Skupno`), never `batch način`; keep the literal `--batch` flag name untranslated when it is being named as a flag.
- Coordinate/axis labels that appear lowercase in a source output template (e.g. `z=` in `step size limited at z=`) stay lowercase — this is a literal program-output token, not a display-convention axis name, so do not upper-case it.
- Reflexive verbs: intransitive "cool down" is `hladiti se` — keep the `se` (e.g. `elementi se hlade`), not bare `hlade`.
