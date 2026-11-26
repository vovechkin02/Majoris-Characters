# Legacy character sets and byte sequences

## Catalog

Characters exposes 77 explicitly supported ICU converters, tested with ICU 76.1.
Existing encoding names and positions are preserved. New entries follow them.
The selected converter does not change the process or system locale.

| Group | Encodings |
|---|---|
| ASCII | US-ASCII |
| ISO-8859 | 1–11, 13–15 |
| Windows | 1250–1258, 874 |
| DOS / IBM | 437, 720, 737, 775, 850, 852, 855, 857, 858, 860–864, 865, 866, 869, 1125 |
| EBCDIC | 037, 500, 1047, 1140 |
| KOI8 | R, U |
| Macintosh | Roman, Cyrillic |
| East Asian independent mappings | Windows-932, Windows-936, Windows-949, Windows-950, GB2312, EUC-KR, Big5-HKSCS (ICU), EUC-JP, GB18030-2022, EUC-TW |
| Stateful | ISO-2022-JP, JP-1, JP-2, KR, CN, CN-EXT; HZ-GB-2312 |
| ISCII | Devanagari, Bengali, Punjabi, Gujarati, Oriya, Tamil, Telugu, Kannada, Malayalam |

## Browsing and conversion

Open **Character Sets**, choose an encoding and search by Unicode name or
`byte:` followed by hexadecimal pairs. `byte:94 39 FC 36` in GB18030-2022
selects U+1F600. EUC-JP includes the three-byte JIS X 0212 grammar; EUC-TW
includes four-byte plane sequences. Independent units are enumerated and
strictly decoded, retaining all scalar alternatives within those grammars.
The grid displays valid mapped Unicode scalars even if they are unassigned in
Unicode 17. Its page size remains 256, not the full repertoire size.

Contextual converters use ICU's round-trip repertoire, validated by encoding
and decoding each scalar independently. The grid displays canonical standalone
bytes, including required shifts/resets, not every possible byte alias. For
example, ISO-2022-JP bytes `1B 24 42 24 22 1B 28 42` decode to U+3042. The
same scalar may occur in other valid stream contexts. A query that produces
multiple scalars or an identity outside the scalar grid directs the user to
**Inspect Byte Sequence**.

That dialog supports:

- Decode a complete hexadecimal byte sequence starting in the initial state.
- Preserve the entered bytes, including shifts and noncanonical aliases.
- Encode explicit `U+` notation or literal text, without normalization.
- Reject encoding unless the complete decoded scalar sequence is identical.
- Copy hexadecimal notation and open the shared Unicode inspector for each
  constituent scalar, original text copying, favorites and composition.

For example, ISCII Devanagari `E8 E8` decodes to U+094D U+200C; `E8 E9`
decodes to U+094D U+200D. GB18030-2022 `41 00 94 39 FC 36` decodes to
U+0041 U+0000 U+1F600. The NUL is preserved in numeric representations;
GTK's text clipboard cannot carry it, so the existing inspector disables
original-text actions for that sequence.

## Implementation boundary

`src/encodings/legacy-workflow.vala` owns hexadecimal parsing, byte and
alias formatting, scalar membership checks, and exact round-trip validation.
The private native adapter accepts and returns explicit-length arrays of bytes
or scalars, and returns ordered packed mapping keys for independent encodings.
Contextual repertoire enumeration still checks each scalar in C to avoid
allocating per-candidate arrays or crossing the language boundary for each
mapping. The existing table ownership and worker lifecycle are unchanged.

## Bounds and semantics

- 1–128 bytes per conversion, written as contiguous hex pairs or with one ASCII
  space between pairs. The UI trims outer whitespace only for hexadecimal input.
- 1–32 valid Unicode scalars per conversion, including NUL. No replacement,
  transliteration, normalization or fallback mappings are requested.
- Each conversion uses an independent ICU converter. No state persists between
  operations. A complete stream may end in a shifted state if ICU accepts it;
  truncated input, malformed sequences and output-free shift sequences are rejected.
- Canonical reverse encoding need not reproduce an original valid alias. The
  decode path never replaces the original byte field with canonical bytes.
- Repertoire construction is bounded to two million independent entries and
  executes on the existing source worker. Search remains cancellable; source
  enumeration itself is not interrupted, and superseded work is never published.

## Limitations

This is an ICU-backed character-set browser and bounded stream converter, not
an exhaustive enumeration of all possible byte strings or state-machine
histories. Contextual multi-scalar mappings are available by explicit conversion,
not listed as independent entries in the scalar grid. ICU's mapping variants
can differ from other specifications (for example WHATWG Big5 or another
platform's HKSCS mappings). ISO-8859-16 is not present in the tested ICU build;
not every historical encoding is exposed. These choices do not imply that
non-Unicode font charmaps are equivalent to legacy text encodings.

See [Vala migration pass 4](vala-migration-pass-4.md) for current evidence and
[Priority 5 qualification](priority-5-qualification.md) for executed checks
and remaining target-system validation.
