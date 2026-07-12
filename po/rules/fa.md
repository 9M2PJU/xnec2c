# fa translation rules

## Script and typography
- Perso-Arabic script, RTL. Use Persian letters ک/ی/گ/چ/پ/ژ, never Arabic ك/ي.
- Use ZWNJ (نیم‌فاصله, U+200C) for compound morphology: می‌شود, نمی‌, ‌های, پیش‌فرض, بارگذاری, etc.
- Western Arabic digits (0-9) for all numbers, not Eastern Arabic-Indic digits.
- Format specifiers (%s, %d, %f, %c, %%), unit symbols (MHz, dBi, Ω), NEC2 mnemonics
  (GW, EX, LD, FR, RP, GE, EN...), and VSWR stay unmodified and LTR inside RTL text.
- No case distinction in Perso-Arabic script.

## Register
- Formal/impersonal written register throughout: imperative "...کنید" / "...را انتخاب کنید".
- Never informal تو-forms or spoken contractions (میخوایم, نیستش).
- GTK `_` hotkey mnemonics: keep the underscore on a Persian letter; shift to another
  letter within the same dialog if a duplicate hotkey would result.

## Established lexicon (from existing catalog; keep consistent)
- current (electrical) → جریان / جریان‌ها
- charge (electrical) → بار / بارها
- patch (NEC2 surface patch) → پچ (transliteration, not پارچه/تکه)
- wire → سیم
- phase → فاز
- frequency → فرکانس
- field → میدان
- polarization → قطبش
- radiation / radiation pattern → تشعشع / الگوی تشعشع
- voltage → ولتاژ
- impedance → امپدانس
- structure / geometry → ساختار / هندسه
- excitation → تحریک
- gain → بهره
- ground plane → صفحه زمین
- scale → مقیاس
- color → رنگ
- brightness → روشنایی
- animate/animation → انیمیشن
- reset → بازنشانی
- default(s) → پیش‌فرض(ها)
- peak → اوج (not پیک); eg "Peak magnitude" → دامنه اوج, "Peak value" → مقدار اوج
- projection (color/render mapping) → فرافکنی; "Color projection" → فرافکنی رنگ
  (not تصویر رنگ, which reads as "picture of color")
- hue → رنگ‌مایه
- knee → زانو
- softening → نرم‌سازی
- compression → فشرده‌سازی
- contrast → کنتراست

## Never translate
- NEC2 card mnemonics: GW, GA, GH, EX, LD, FR, RP, GE, EN, SP, SM, NE, NH
- VSWR, dBi, S-parameters, Ω, MHz, dB
- File extensions: .nec, .csv, .s1p, .s2p, .png
- Function/identifier names embedded in developer-facing pr_* messages (eg
  config_widget_lookup, mem_array_free) — keep verbatim, translate only the
  surrounding sentence.

## Disambiguation
- Use plain domain term without adding "الکتریکی" (electrical) qualifier unless the
  English source itself qualifies it; program context (NEC2 simulator) already
  disambiguates جریان/بار as electrical.
