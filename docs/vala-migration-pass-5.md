# Vala migration pass 5 — 1.0.13

## Scope and ownership

`FontFaceRecord` copies validated family, raw style sort key, display style,
filesystem path, face/instance index and capability flags from the discovered
Fontconfig pattern. `FontInventory` owns the raw native catalog, the ordered
face records and the family-offset vector. It rejects the same missing or
malformed identities as the previous C implementation and sorts bytewise by
family, raw style, path, and exact numeric index. Missing style sorts as an
empty string; malformed style is excluded from display but retains its raw
sort key. Filesystem paths are not required to be valid UTF-8. Equal sort keys
retain discovery order; the previous C sort did not guarantee a tie order.

The inventory builds on the existing discovery worker, publishes immutable
records through `FontSourceRequest`, and preserves the existing freshness and
supersession contract. Selection restoration, family grouping and the scalar
comparison filter use one inventory. Comparison matching retains the previous
case-folded family/style label and exact coverage test; comparison filtering
still runs in the existing main-context path. Application positions never
become raw Fontconfig indexes: every native description, coverage query,
exact-face opening and nominal face match resolves the stored native index.
An exact face is independently owned and transferred once from the source
worker as before. The native catalog returns its raw `FcFontSet` directly,
without a second filtered set or a native sort. Raw metadata may be absent;
Pango identity comparison now guards against missing file/index metadata.

The native adapter retains Fontconfig configuration ownership, pattern access,
coverage queries and PangoFc integration. FreeType rasterization, variation
coordinates and cmap enumeration are unchanged. This is direct composition
of an immutable inventory and the existing library adapters, not an additional
pattern hierarchy. The added records scale with installed faces, not Unicode
scalars. No dependency, schema, UI template or public interface is introduced.
The allocation and refresh cost of the metadata records and linked-list sort
has not been benchmarked.

## Verification in the delivery environment

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| Native ICU adapter | 3 groups passed |
| Prepared search | 6 groups passed |
| Native legacy adapter | 3 groups passed |
| Native Unicode compilation | GCC, ICU 76.1, `-Wall -Wextra -Werror` |
| Python source, protocol and generator checks | 21 passed |
| Declaration documentation lexical check | 1,593 of 1,593 detected declarations |
| Unicode data, generated and upstream sources | All 37 byte-identical to 1.0.12 |
| Font catalog C compilation and tests | Not run; development headers unavailable |
| Vala inventory and full mixed-language tests | Not run; Vala compiler unavailable |
| GTK runtime, performance and Debian binary package | Not run |

The existing font-source suite adds an inventory-policy test covering the
complete raw-to-sorted index mapping, bytewise comparator, family boundaries,
style validation, flags, nominal description mapping, exact coverage filtering,
selection restoration and one-time worker result transfer. The native catalog
suite now tests raw discovery rather than imposing application ordering. These
font tests are wired into the existing targets, not represented as executed.
No native font compilation or Vala runtime equivalence is claimed. The native
Unicode regression results do not qualify the changed font subsystem.

This is a source migration candidate. Run on the intended Debian 13 system:

```sh
python3 tools/qualification/qualify.py build-release --package
```

## Remaining migration

The next step is to establish a full compiled baseline, then review native
adapter consolidation and measure search traversal before migrating any
remaining hot loops. Preserve explicit-length arrays, raw identity mapping,
renderer ownership, deterministic ordering and cancellation semantics. Do not
move generated tables or FreeType operations merely to improve language counts.
