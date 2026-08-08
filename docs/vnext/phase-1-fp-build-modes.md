# Phase 1 floating-point build modes

## Outcome

```text
BASE_COMMIT=fa41561a1f28045df61e19ad0cfe6bed2de2786e
IMPLEMENTATION_COMMIT=abeca81bd1ed5a69fd1b3ca6d639282723fbc169
LEGACY_FAST_BUILD=GREEN
STRICT_FP_BUILD=GREEN
DEFAULT_BEHAVIOR_CHANGED=false
STRICT_FAST_NUMERICAL_COMPARISON=RED
OMNICORE_FAST_MATH_ALLOWED=false
G0_UPSTREAM_BASELINE=RED
```

The project now has an explicit `-Dfp_mode=legacy_fast|strict` Meson option. The
default is `legacy_fast`, preserving existing optimized-build behavior. `strict` is
a whole-project numerical-validation build until dedicated OmniCore targets exist.

## Files changed

- `meson_options.txt`: declares the two-value option and Legacy-compatible default.
- `meson.build`: separates optimization/vectorization flags from the floating-point
  contract and prints the selected mode at configure time.
- `tools/tests/test_fp_build_modes.py`: four fail-closed source-contract tests.

No Simulation, Particle, Air, element, Lua, Save or runtime data structure changed.

## Flag verification

Three fresh Meson directories used GCC 16.1.0, `debugoptimized`, `-O2`, SSE2,
`lto=false`, static prebuilt dependencies and `resolve_vcs_tag=no`:

| Build | Option | Compile commands | Fast/unsafe flags | Strict flags |
|---|---|---:|---:|---:|
| default | option omitted | 754 | 754 / 754 | 0 |
| Legacy | `legacy_fast` | 754 | 754 / 754 | 0 |
| Strict | `strict` | 754 | 0 / 0 | 754 each |

Both explicit builds retain `-O2`, `-msse2`, `-ftree-vectorize` and
`-fomit-frame-pointer` in all 754 commands. Strict adds:

```text
-fno-fast-math
-fno-unsafe-math-optimizations
-ffp-contract=off
```

The MSVC branch selects `/fp:fast` or `/fp:strict`, but MSVC was not compiled in this
phase and remains `not_tested`.

## Build and test evidence

| Check | Legacy-fast | Strict |
|---|---:|---:|
| clean-directory build | `786/786 PASS` | `786/786 PASS` |
| Meson static suite | `39/39 PASS` | `39/39 PASS` |
| nested Python tests | `248/248 PASS` | `248/248 PASS` |
| 100.1 Lua boundary cases | `11/11 PASS` | `11/11 PASS` |
| Lua module/high-ID allocation | covered by prior default build | `PASS` |

Post-commit run-specific artifacts after the last relink:

| Build | Bytes | SHA-256 |
|---|---:|---|
| Legacy-fast | 321,336,060 | `7CEFEC225C985B26688BA6F6893E2000CC4B2C838C43139D3B9DD1E02730F1E7` |
| Strict | 321,353,783 | `C79D674A7C7A1AE6BA08C4AF0E2E9EDEF01CB26662752204BF7D1D3F8432BDFF` |

These hashes identify the tested local artifacts; they are not a reproducibility
claim. The generated-data targets relink on subsequent compile invocations and the
PE output is not bit-reproducible under this development configuration.

## Runtime portability boundary

The first isolated Strict Lua launch exited with Windows status `0xC0000135`. The
development executable imports `libgcc_s_seh-1.dll`, `libstdc++-6.dll` and
`libwinpthread-1.dll`; `-Dstatic=prebuilt` alone does not statically link the GCC
runtime. Re-running with `C:/msys64/ucrt64/bin` explicitly on PATH passed all Lua
checks above.

Therefore:

```text
STRICT_DEVELOPMENT_RUNTIME=PASS_WITH_UCRT64_PATH
STRICT_PORTABLE_RUNTIME=not_tested
```

Release/portable validation must use the documented static GCC runtime link flags and
a clean-machine test. This phase makes no portability or release claim.

## Numerical, performance and memory evidence

- Numerical conservation/drift comparison: `not_tested`; no OmniCore solver exists.
- NaN/Inf, positivity and floor comparison: `not_tested`.
- Throughput before/after: `not_tested`; compilation/test duration is not a benchmark.
- Runtime RAM/VRAM before/after: `not_tested`.
- Static data-structure memory change: zero.

Passing the same Legacy static/Lua checks in both modes proves build viability, not
numerical equivalence and not that fast-math is safe.

## Compatibility, risks and rollback

Default configuration was directly tested and resolves to `legacy_fast`, so existing
optimized builds retain their prior flags. Debug (`optimization=0/g`) still avoids
the Legacy fast flags; selecting `strict` explicitly adds the strict contract.

Remaining risks are compiler/platform coverage, external user-supplied flags,
prebuilt dependency FP behavior and the absent fixed-step numerical runner.

Rollback commit is `fa41561a1`. Reverting `abeca81bd` removes only the option and its
contract test; it does not require save/data migration.
