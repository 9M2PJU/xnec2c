# cs translation rules

## 1. Character set, symbology, expectations
- Latin script with diacritics: á é í ó ú ů ý ě š č ř ž ď ť ň.
- Decimal separator is comma: `50,0` not `50.0`.
- Thousands separator is a space, not comma.
- Non-breaking/normal space between a number and its unit: `10 MHz`.
- No case-distinction concept beyond standard capitalization rules; Czech does not capitalize common nouns.

## 2. Technical interface writing standards
- Sentence case for labels and menu items, not English-style Title Case.
- Commands and menu actions use the impersonal infinitive form (GNOME/KDE Czech convention): "Uložit", "Otevřít", "Zobrazit" — not imperative "Ulož" or gerund forms.
- Status/error messages use impersonal or third-person phrasing.
- Concise, compact technical phrasing; avoid padding.
- Prefer established Czech technical terms over anglicisms where a standard equivalent exists (e.g. "soubor" for file); keep widely-adopted EE/RF loanwords where no natural Czech term is standard.

## 3. Formality / register
- Czech distinguishes informal "ty" vs formal "vy" in 2nd-person verb conjugation.
- Default: impersonal/infinitive command style avoids the ty/vy choice entirely (see §2).
- Where direct 2nd-person address is unavoidable (e.g. confirmation dialogs), use formal "vy" conjugation; never informal "ty".

## 4. NEC2 / EM-simulator domain mapping
### Never translate
- NEC2 card mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN
- File extensions: .nec, .csv, .s1p, .s2p, .png
- Unit symbols: MHz, dBi, Ω, VSWR
- "NEC2", "Smith" (chart name)

### Domain term glossary (electrical sense; keep consistent everywhere)
| English | Czech | Notes |
|---|---|---|
| current | proud | electrical (Amperes), never "aktuální/nynější" |
| charge | náboj | electrical charge (Coulombs) |
| ground / ground plane | zem / uzemnění / zemní rovina | context-dependent, never "půda" |
| wire | vodič | never "kabel/šňůra" |
| gain | zisk | antenna directivity, never "zisk" as profit |
| load | zátěž | electrical impedance, never "váha/náklad" |
| excitation | buzení | RF excitation source |
| segment | segment | NEC2 geometry term, unchanged |
| patch / surface patch | ploška | plural/list contexts: "plošky"; short standalone menu label keeps existing catalog form "Patch" |
| radiation pattern | (vyzařovací) diagram / charakteristika | never "vzor/šablona" |
| feedpoint | napájecí bod | |
| impedance | impedance | unchanged |
| viewer (observer of pattern/gain) | pozorovatel | never "pohled" (that is "view" the noun, not the person) |
| color (as a projection axis) | barva | not "měřítko" (that word is reserved for "Scale") |
| color scale / scale family | (barevná) škála / rodina škál | the magnitude-to-color transfer family selector |
| patch, plural checkbox/toggle label | plošky | eg "Patches" toggle; standalone menu label keeps "Patch" |
| named color-transfer functions (Asinh, μ-law, Reinhard, Sigmoid, Power) | unchanged proper names | "Power"→"Výkon" is the exception (translated, not a proper name) |

### Internal/debug message convention
Messages carrying a function- or subsystem-name prefix (eg `config_widget_apply_element: ...`, `mem-report %s: ...`, `theme: cannot load resource %s: %s\n`) are developer diagnostics (LOW priority per TRANSLATING.md) and are kept identical to the English msgid. Generic operator-facing warnings without a code-identifier prefix (eg inotify status lines, validation-mode notices) are translated normally.

Exception: where a nearest sibling family within the same subsystem already translates its message body (eg the `render_settings_init: failed to load ... glade: %s\n` family in src/settings/render_settings_common.c), match that established sibling convention for new entries of the same shape (`prefix: failed to load X: %s\n`) rather than defaulting to fully-identical; local precedent within the same function/module outranks the generic default when one exists.

### Adjective gender agreement for mode labels
Standalone adjective labels agree with their implied or sibling noun, never a default masculine:
- "Instantaneous" as a color-projection/phase mode sits beside "Animated"→"Animované" and "Static"→"Statické" (neuter, implied *zobrazení*), so it is "Okamžité", not masculine "Okamžitý".
- "Instantaneous (φ=0)" pairs with "Peak value"→"Špičková hodnota" (feminine *hodnota*), so it is "Okamžitá (φ=0)"; keep the leading `_` hotkey where the msgid carries one.

### Math variable / coordinate casing
A lowercase math variable or coordinate in the msgid (eg `z=`, `z = 0`) stays lowercase; it is a variable name, not a UI axis label, so the X/Y/Z-uppercase convention does not apply (eg "Velikost kroku omezena na z=", "Hankel není platný pro z = 0").

### Disambiguation policy
No qualifier inflation: translate the domain term alone as the source does (e.g. "Proudy" for "Currents"), never add "elektrické" unless the English source itself qualifies it. Program context (an EM simulator) already disambiguates.
