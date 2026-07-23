# hu translation rules

## 1. Script, symbology, locale data
- Latin script with Hungarian diacritics: á é í ó ú (acute), ő ű (double acute, Hungarian-only — never substitute ö/ü), ö ü (umlaut).
- Decimal comma in locale-formatted numbers; source literals, unit symbols (MHz, dBi, Ω, %, dB) and NEC2 mnemonics stay unchanged.
- Quotation style „lower-upper” when quoting is needed in-string.
- Capitalization: sentence case only (first word + proper nouns); never capitalize every noun.

## 2. Technical UI register
- Button/menu labels: deverbal noun style (Mentés, Törlés, Bezárás, Visszaállítás) preferred over bare imperative verbs.
- Sentence case throughout; no title case.
- Keep locked acronyms/terms untranslated: VSWR, dBi, NEC2, S-parameter, GW/GA/GH/EX/LD/FR/RP/GE/EN, patch (label) kept as "Patch".
- Pro-drop: omit subject pronouns when the verb conjugation carries the person/number.
- GTK accelerators: no duplicate hotkey letter within one menu. In the Frequency Plots `_View` menu "_S11" keeps `S`, so "_Color Theme" shifts to `Szín_téma` (t).

## 3. Formality
- Formal register = 3rd-person-singular/plural verb conjugation implying omitted "Ön/Önök"; never insert the pronoun explicitly, never use informal "te" forms.
- Instructions/help text: polite imperative without pronoun ("Válassza ki…", "Adja meg…").
- Confirmation dialogs: "Biztosan ... szeretne ...?" pattern.

## 4. NEC2 domain lexicon (established + this pass)
| English | Hungarian | Notes |
|---|---|---|
| current | áram | electrical, not "jelenlegi" |
| charge | töltés | electrical charge |
| gain | nyereség | antenna directivity; established catalog term (antennanyereség). NOT "erősítés" (that is amplifier gain) |
| magnitude (of field/current quantity) | nagyság | color-projection / current / charge level |
| amplitude / peak magnitude | amplitúdó | oscillating-quantity peak (eg "Peak Magnitude", color-projection default "amplitude") |
| impedance magnitude \|Z\| | magnitúdó | eg "Z-magnitúdó", "Impedancia (magnitúdó/fázis)"; NOT "abszolút" |
| excitation | gerjesztés | EM excitation |
| impedance | impedancia | |
| phase | fázis | |
| wire | huzal | electrical conductor; NOT "vezeték" — reserve one term. Applies in compounds: huzalsugár, huzalszegmens, kúpos huzal |
| radial / radials | radiális / radiálisok | ground radial wires; standardized to "radiális", NOT "sugárirányú" (eg "Radiális huzalernyő", "…+ radiálisok") |
| segment | szegmens | |
| patch (noun/label) | Patch | kept as-is per existing catalog usage |
| tag | címke | |
| ground plane | földsík | |
| radiation pattern | sugárzási diagram | |
| wireframe | drótváz | |
| peak value | csúcsérték | |
| scale/scaling | méretezés | |
| visualization | vizualizáció | |
| color projection | színvetület | projection of quantity onto color |
| color scale / scale family | színskála / skálacsalád | |
| brightness | fényerő | |
| gamma | gamma | kept, math exponent symbol |
| dynamic range | dinamikatartomány | |
| softening / knee | lágyítás / térdpont | soft-compression curve terms |
| compression | kompresszió | |
| contrast | kontraszt | |
| instantaneous | azonnali | |
| polarity | polaritás | |
| hue | árnyalat | |
| flow / flow direction | áramlás / áramlás iránya | patch current flow |
| feedpoint | tápponton / betáplálási pont | RF feedpoint |
| port | port | kept |
| widget | widget | kept (GTK term, established in catalog) |
| managed allocator | felügyelt foglaló | mem_track terms |
| validation | validáció / ellenőrzés | context: validation_dump = validáció |
| overlay (wire-marker toggle) | rárajzolás | Nodes/antinodes, Comet toggles |
| node / antinode (current) | csomópont / anticsomópont | current-null / current-peak markers |
| comet (moving-crest marker) | üstökös | eg "Comet" toggle, "üstökösfej" = comet head |
| identity (transfer curve) | azonosság | scale-family "Identity" curve, n^1 passthrough |
| sentinel (defensive/unreachable value) | őrérték | eg "unreachable ... sentinel" internal messages |
| tone-map / tone mapping | tónusleképezés | HDR photographic term (Reinhard curve) |
| color tone (internal, = scale family) | skálacsalád | src/color/color_tone.c messages; keep same term as "Scale family:" label, NOT "színcsalád" |
| polarity (instantaneous sign) | polaritás | current/charge sign; NOT "polarizáció" (EM wave polarization is a different concept, see _Polarization) |
| instantaneous (bare, projection name) | azonnali | plain "Instantaneous" projection label; add "(φ=0)" qualifier only when source includes it |
