# Native GLib standardization — 1.0.15

## Scope

This pass implements the first priority of the revised native migration plan.
It standardizes the 11 handwritten native C implementation modules and their
project-owned headers, without changing generated or upstream Unicode data,
search ordering, application settings, or the external picker protocol.

Both static libraries and their exported Meson dependencies now explicitly
require GLib 2.80 or later. Debian already declares the corresponding
`libglib2.0-dev` dependency. The local `gunichar` redefinition is removed.

Project-owned Boolean, string, length, numeric and scalar contracts use GLib
spellings. `gunichar` denotes scalar values; opaque search identities, packed
predicates and raw symbol codes remain fixed-width integers. ICU lengths and
third-party structures retain the types required by their APIs. Vala scalar
arrays remain `uint32[]`, which shares the underlying `guint32` representation
with `gunichar`. VAPI array length metadata explicitly uses `gsize` or `gint`.
Native test callers and Vala-exported test declarations use corresponding types.
This is an internal C ABI change for Boolean values; rebuild all dependent
objects together rather than mixing objects from older versions.

## Ownership and performance

All handwritten native allocations formerly made with the C standard allocator
now use GLib allocation and `g_free()`. Recoverable paths retain their NULL
checks and search failure status. The private `characters_try_strdup()` helper
uses `g_try_malloc()` rather than introducing fatal allocation behavior through
`g_strdup()`. Typed arrays use checked multiplication through `g_try_malloc_n()`
or `g_try_realloc_n()`; fixed-size objects use `g_try_new0()`. The font adapter's
preexisting GLib container and raster allocation semantics are retained.

The native scanner retains compact vectors, the bitset, C11 relaxed atomics,
and inline ASCII folding in its hot loop. Locale tag comparison uses
`g_ascii_strcasecmp()`; generated annotation tables retain a compatibility
`COUNT` macro backed by `G_N_ELEMENTS()`. Formatting uses `g_snprintf()`.
External object destructors remain paired with their originating libraries.
No new GObject hierarchy or per-character heap object is introduced.

Header include groups and function-body brace placement have been standardized.
The project-specific native rules are recorded in `CONTRIBUTING.md`.

## Verification

| Check | Result |
| --- | --- |
| Native Unicode engine | 6 groups passed |
| ICU adapter | 4 groups passed, including 17,086 direct decompositions |
| Prepared search | 6 groups passed |
| Legacy adapter | 3 groups passed |
| Python source, protocol and import tests | 23 passed |
| Documentation lexical check | 1,597 of 1,597 declarations |
| Baseline comparison | 300,543 catalog identities; 1,819 supplementary emoji identities; 77 ordered encoding repertoires and sampled byte keys matched 1.0.14 |
| Unicode source, generated and upstream files | 37 byte-identical files |
| XML, JSON, Python syntax, GSettings and Debian changelog | Passed |
| Standard GLib header compilation | Not run |
| Font native compilation, Vala build and full Meson suite | Not run |
| Debian binary package and performance qualification | Not run |

The native tests and comparison harness were compiled with GCC, `-Wall -Wextra
-Werror`, ICU 76.1 development headers, and temporary minimal declarations for
the GLib types, macros and functions used by the Unicode adapter. The resulting
executables linked to the installed real GLib 2.84 runtime. The environment has
no usable GLib development headers or pkg-config metadata, and package mirror
DNS resolution failed. The temporary declarations and harness are not part of
this source archive; they do not replace compilation against the actual GLib
headers. The font development headers and Vala compiler are also unavailable.

The 1.0.14 comparison used the prior native probe output and the same catalog
and encoding traversal with GLib-owned result cleanup. It is not a full GUI,
Vala, font, or performance equivalence test.

Run the established qualification gate on Debian 13 before release:

```sh
python3 tools/qualification/qualify.py build-release --package
```

## Remaining migration

Next migrate character presentation and collection definitions to Vala,
including shared code-point formatting, placeholder/display decisions and
collection label policy. Subsequent work covers remaining Unicode relationship
construction, emoji annotation policy, and non-hot-path property formatting.
Retain native search traversal until a compiled performance baseline exists.
