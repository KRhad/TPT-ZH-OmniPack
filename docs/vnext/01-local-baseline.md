# Local baseline

## Repository identity

| Field | Value |
|---|---|
| Workspace | `D:/CodexWork/OmniPack/repos/TPT-ZH-OmniPack` |
| User junction | `C:/Users/KR/TPT-ZH-OmniPack` |
| Pre-vNext OmniPack | `fb72d5e8f` |
| Original common base | `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd` |
| Current integration HEAD | `f1320b48dfd5a570edc353a6d52dcd8adc094345` |
| Current VCS tag | `v100.1.400-186-gf1320b48d` |
| Stable/master upstream | `d768aeb89acad986bd252d7e904bf44bb374545f` |
| Particle layout | 56-byte AoS, `NPART=235008` |
| Resolution | 612 x 384 particles; 153 x 96 Air cells; `CELL=4` |
| Graphics/input dependency | SDL2 `2.30.9-tpt-libs` |

## Current build artifact

The clean build directory is `build-vnext-g0-clean`. The original clean build
completed `787/787` actions. After the tooling-only inventory commit, the executable
was regenerated and linked successfully (`66/66`) with Windows Git first on PATH.

```text
EXE=build-vnext-g0-clean/tpt-zh-omnipack.exe
EXE_BYTES=321332476
EXE_SHA256=EA9638B84003D93454B8773CA61A8F6FB7772108982C5C20554E0ABC97D6CA6B
VCS_TAG=v100.1.400-186-gf1320b48d
STATUS_COUNT_AFTER_LINK=0
```

GCC emitted a `-Wmaybe-uninitialized` warning in the `ByteString`/`optional` path in
`PowderToy.cpp`; this is recorded as `needs_triage`, not as a proven runtime defect.

## Test evidence

| Check | Result | Scope |
|---|---:|---|
| Meson registered tests | `39/39 PASS` | current clean build; includes nested project audits |
| Nested Python unit tests | `244/244 PASS` | audit/generator tests |
| Lua module regression | `PASS` | isolated client process |
| Lua 100.1 boundary regression | `11/11 PASS` | reset pressure/velocity/neighbors boundaries |
| OPS scenario cases | `8 PASS` | fixed scenario set |
| OPS processes | `24` | three processes per scenario |
| OPS restarts/load verifications | `16 / 16` | two restart loads per scenario |
| OPS particles | `368` total | scenario aggregate |
| OPS field assertions | `436` per load | stable identifier and carried fields |
| Element inventory check | `PASS` | 488 elements, 431 bindings, 164 roots |
| GUI visual acceptance | `not_tested` | no visible interactive review |
| PSv/fuC fixtures | `not_tested` | current fixtures are OPS1 |
| Portable clean-machine runtime | `not_tested` | compile is not portability proof |
| Broad third-party Lua corpus | `not_tested` | only bounded local regressions |
| Current uncapped throughput | `not_tested` | no accepted kernel benchmark |
| Conservation/numerical drift | `not_tested` | no OmniCore solver exists |

## G0 gate

```text
BUILD=GREEN
STATIC_TESTS=GREEN
LUA_BASIC=GREEN
LUA_100_1_BOUNDARIES=GREEN
OPS_ROUNDTRIP=GREEN
LATEST_STABLE_CODE_PRESENT=GREEN
CRITICAL_UPSTREAM_FIXES_PRESENT=GREEN
GUI_VISUAL=NOT_TESTED
PSV_FUC=NOT_TESTED
PORTABLE_RUNTIME=NOT_TESTED
CURRENT_UNCAPPED_BENCHMARK=RED
CHARACTERIZATION_SAVES=RED
STRICT_FP_NUMERICAL_BASELINE=RED
DIFFERENTIAL_RUNNER=RED
G0_UPSTREAM_BASELINE=RED
```

The gate is fail-closed: passing build and compatibility sub-gates does not substitute
for the four missing numerical/performance foundations.
