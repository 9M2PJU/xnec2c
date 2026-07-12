# it translation rules

## 1. Native character set, symbology, expectations

- Latin script with diacritics: à, è, é, ì, ò, ù, ç (rare). Grave accent dominates (è, à, ù); acute appears in perché-type words and word-final é (poté).
- Apostrophe elision is standard and required: l'ampiezza, dell'ottimizzatore, sull'antenna — never "la ampiezza".
- Guillemets («»)) exist in print Italian but this catalog already uses straight double quotes/apostrophes in UI strings; match existing catalog usage rather than introducing «».
- Unit symbols, NEC2 mnemonics, and abbreviations stay as in source and are never transliterated: MHz, dBi, Ω, VSWR, GW, GA, GH, EX, LD, FR, RP, GE, EN, .nec, .csv, .s1p, .s2p, .png. The catalog also keeps bracketed English unit abbreviations verbatim where already established, e.g. "Angolo Eta (deg)".
- No decimal-comma formatting appears inside translatable UI text in this catalog (numbers arrive via format specifiers at runtime); do not rewrite `%f`/`%d` output formatting in msgstr.

## 2. Technical interface writing standards

- Menu items, button labels, and column/field titles use Title Case in this catalog (established precedent: "Guadagno Netto", "Diagramma di Radiazione", "Impedenza Caratteristica", "Scala Struttura"). Preserve this pattern for new label-type strings; use normal sentence case for full sentences, tooltips, and dialog/error messages.
- GTK mnemonics: keep the underscore on the same accelerator letter used by the established Italian string family when the msgid is a variant of an already-translated label (e.g. "_Istantaneo (φ=0)", "_Ampiezza di Picco", "_Asse Polarizzazione"); when introducing a new hotkey, pick a letter that does not collide with mnemonics already assigned in the same menu/dialog and is natural for the Italian word.
- Error/log/debug strings (pr_debug, pr_err, mem-report, theme:, config_widget_*, validation:) are short, literal, technical sentences; keep function/variable names, printf tokens, and internal identifiers (e.g. `mem_array_free`, `config_widget_lookup`) unmodified inside the Italian sentence.
- Tooltips are full descriptive sentences using sentence case and formal register (see §3); they explain the reason for greyed-out or disabled controls when the source does.

## 3. Formality and informality mapping

- Register: impersonal form for any sentence addressing the user directly — dialogs, confirmations, tooltips, instructions. Use the infinitive for imperative-style instructions ("Selezionare nuovamente la riga", "Regolare γ nella finestra di animazione", "Verificare le schede geometriche", "Scegliere tra vettori...") and the "si" impersonal for questions/statements ("Si desidera uscire da xnec2c?"). Never use "tu" forms (no "Seleziona tu...", no "devi"); do not switch to the Lei imperative ("Selezioni") either — the established catalog register is impersonal, matching Italian GTK/GNOME localization practice (this intentionally overrides the generic "formal Lei" note in doc/TRANSLATING.md). A control-describing sentence keeps the third-person indicative of the source ("Controls whether..." → "Controlla se...", "Mirrors..." → "Riproduce...", "Reveals..." → "Rivela..."); this is descriptive, not a "tu" imperative, and stays as is.
- Commands/menu items/button labels use the imperative-as-command form already established in the catalog ("Salva", "Apri", "Chiudi", "Annulla", "Applica") — this is the standard GNOME/GTK Italian localization convention and is register-neutral (works for both tu/Lei), so it is not a Lei/tu conflict; keep using it for new short commands.
- Error and debug messages are impersonal/technical statements (no direct address to the user), matching the existing catalog style: state the condition, not a command to the user.
- Sections 1-3 do not overlap: §1 is script/symbol mechanics, §2 is layout/casing/mnemonic convention, §3 is grammatical person/register for sentences that address the user.

## 4. Domain mapping (NEC2 electromagnetic simulator, Italian/Italian amateur-radio culture)

- corrente / correnti = electrical current (never "attuale/presente"); already used bare, without an "elettrica" qualifier, matching source qualification level (e.g. "Correnti", "Visualizzazione _Corrente").
- carica / cariche = electrical charge (Coulomb), never "addebito/costo".
- massa / piano di massa = electrical ground / ground plane (never "terreno/suolo" except when the NEC2 GN/GD ground-card sense is meant, already rendered "Massa (GN)" / "Massa (GD)").
- filo / conduttore = thin antenna wire element (never "cavo" in the structural-element sense).
- guadagno = antenna gain ratio (dB/dBi), never "profitto".
- eccitazione = electromagnetic excitation/source (never emotional sense).
- carico (load) = electrical impedance load (never "peso/carico fisico").
- diagramma di radiazione = radiation pattern (never "modello/template").
- radiali = ground-plane radial wires (never "relativo al raggio").
- Keep VSWR, dBi, S-parametri (S11, S1P/S2P), Z, impedenza as the locked technical terms already used throughout (Impedenza Caratteristica, Impedenza vs Frequenza, Diagramma di Smith).
- Amateur-radio audience: use terminology consistent with Italian radioamatori usage (ARI-aligned), e.g. "diagramma di radiazione", "guadagno", "adattamento d'impedenza", favoring the term already fixed in this catalog over a merely-common synonym.

## 5. Status

File created; scoped entries translated under this rule set; extend with new terms only when a future scoped batch introduces them.
