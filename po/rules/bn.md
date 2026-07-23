# bn translation rules

## Script and numerals
- Bengali script (Unicode Bengali block). No uppercase/lowercase concept: GTK `_` hotkey
  mnemonics mark a distinguishing character, not a case variant.
- Keep Western numerals (0-9) for all technical values, format-specifier output
  (`%d`, `%f`, etc.), and units. Do not use Bengali numerals (০-৯) in technical strings.
  This includes ordinal designations of technical entities (eg "1st/2nd Medium" →
  "1ম/2য় মাধ্যম", keeping the Western digit with the Bengali ordinal suffix).
- Sentence-final "।" (dari) is used for full standalone prose sentences, matching
  existing catalog entries. UI fragments/labels keep Latin punctuation as already used.

## Technical lexicon (transliteration convention)
Reuse these established transliterated forms consistently; do not coin alternate
pure-Bengali synonyms for the same meaning:
- current → কারেন্ট
- charge → চার্জ
- phase → দশা
- excitation → এক্সাইটেশন
- impedance → ইম্পিড্যান্স
- widget → উইজেট
- config → কনফিগ
- thread → থ্রেড
- port → পোর্ট
- scale → স্কেল
- gain → গেইন
- flow → প্রবাহ (eg "Flow direction" → "প্রবাহের দিক"; not for current itself, which stays কারেন্ট)
- render → রেন্ডার (verbal/gerund রেন্ডারিং)
- projection → প্রক্ষেপণ
- radiation → বিকিরণ (native term, used in all menu/title/axis labels; never transliterate
  as রেডিয়েশন. Applies to compounds: "radiation pattern" → বিকিরণ প্যাটার্ন,
  "radiation resistance" → বিকিরণ রেজিস্ট্যান্স, "radiated" → বিকিরিত)
- near (near field/near-field) → নিকট (pairs with far → দূর: নিকট ক্ষেত্র / দূর-ক্ষেত্র);
  never নিকটবর্তী
- animated (mode/adjective) → অ্যানিমেটেড (verb stem অ্যানিমেট only for the imperative
  "Animate" action)
- color → রং (bare noun uses anusvara: রং, not রঙ; genitive is written রঙের)

Do not add a qualifying adjective (eg "বৈদ্যুতিক"/electrical) that is absent from the
English source; program context (an EM simulator) disambiguates already-technical terms
like কারেন্ট/চার্জ without needing "বৈদ্যুতিক কারেন্ট"/"বৈদ্যুতিক চার্জ".

## Formality
- Use আপনি (formal you) and the formal imperative ending "-উন" (করুন, নির্বাচন করুন,
  ব্যবহার করুন) throughout. Never তুমি/তুই.

## Format specifiers
- Preserve every `%s`/`%d`/`%f`/`%c`/`%%` from the msgid, in the same order, in the
  msgstr, even when Bengali sentence structure differs (free word order allows natural
  placement around fixed specifier positions).
