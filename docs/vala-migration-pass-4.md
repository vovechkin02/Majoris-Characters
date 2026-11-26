# Vala migration pass 4 — 1.0.12

## Scope and ownership

The legacy encoding workflow moves to `src/encodings/legacy-workflow.vala`.
It owns hexadecimal parsing, formatting, scalar membership policy, and explicit
sequence round-trip validation. The browser and sequence dialog share this
workflow. Original entered byte notation remains preserved by the dialog.

The native adapter (`native/unicode/legacy-encoding.c`) owns converter opening,
strict callbacks, explicit-length UTF-16 conversion, and compact sorted
repertoire enumeration. It publishes owned raw byte/scalar arrays or packed
mapping keys through the private VAPI; table identity and membership are
read-only. No unbounded C-string processing of raw bytes is necessary, so
embedded zero remains valid. The 77 encoding choices and stable indexes are
unchanged. The 128-byte and 32-scalar limits remain enforced by the native
adapter, with validation at the Vala policy boundary as appropriate.

Encoding is accepted only when the reverse conversion reproduces every scalar.
ICU may silently omit default-ignorable scalars even under strict callbacks;
for example, ASCII encodes `A` plus U+200D as only `41`. The Vala workflow
rejects this result. Native contextual repertoire construction retains its
allocation-free scalar round-trip check for the same reason. Independent
mapping keys preserve the existing ordering by byte length and unsigned
value; Vala adds the original comma and space separators. No GObject is
allocated per mapping. This is the existing focused Adapter boundary with
composed domain operations, not a new Strategy hierarchy. No new runtime
package, public interface, UI template, or settings change is required.

The old C hexadecimal parser and formatter and explicit sequence-policy
functions are removed. The mixed C fixture suite calls the production Vala
functions through a test-only declaration header. The native adapter suite
covers bounded arrays and raw mapping ownership independently. The C fixtures
retain their existing seven groups and add byte-alias and lossy-encoding
assertions. The native repertoire tests retain the original search fixtures.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native legacy adapter | 3 groups passed |
| Native Unicode engine | 6 groups passed |
| Native ICU adapter | 3 groups passed |
| Prepared search | 6 groups passed |
| Repertoire comparison with 1.0.11 | All 77 ordered scalar arrays identical |
| Sampled mapping comparison with 1.0.11 | 2,342 byte representations identical |
| Native C compilation | GCC, ICU 76.1, `-Wall -Wextra -Werror` |
| Python source and generator tests | 20 passed |
| All 37 Unicode data files | Byte-identical |
| Full mixed Vala/C tests, GTK runtime and Debian binary build | Not run |
| Vala workflow allocation and performance parity | Not measured |

The comparison harness is temporary and not distributed. It independently
formats native mapping keys and checks contextual raw conversion against the
prior C implementation. It does not execute the Vala workflow. In particular,
explicit encoding now creates independent strict converters for encoding and
reverse decoding; the small per-conversion overhead and grid formatting cost
remain to be benchmarked. The full Vala/GTK development toolchain is not
installed in the delivery environment. This version is a source migration
candidate, not a compiler-qualified release. Run on Debian 13:

```sh
python3 tools/qualification/qualify.py build-release --package
```

## Remaining migration

Qualify the existing Vala migrations before further changes. The next separable
area is font inventory policy, leaving Fontconfig and FreeType native adapters
and compact repertoire enumeration intact. Consolidate native contracts only
where ownership or design benefits are demonstrated, not to reach a language
percentage quota.
