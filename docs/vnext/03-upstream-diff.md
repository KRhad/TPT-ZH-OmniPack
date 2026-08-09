# Upstream diff snapshot

The canonical four-way analysis is [`upstream-diff.md`](upstream-diff.md). This
numbered report fixes the first-round conclusions to the current integration point.

| Comparison | Local-only | Other-only | Meaning |
|---|---:|---:|---|
| common base `bff38ce` vs pre-vNext `fb72d5e8f` | 181 | 0 | OmniPack development |
| pre-vNext `fb72d5e8f` vs stable `d768aeb89` | 181 | 13 | fork had local work and lacked 13 upstream commits |
| current `430b3bd28` vs stable/master | 214 | 0 | upstream remains integrated; 2026-08-10 refresh found no new official commits |

All 13 previously missing upstream commits are classified in the canonical report.
The 1.0.2 official website/release/Git refresh found stable and master unchanged at
`d768aeb89`; no new change impact or adaptation was required. `G0_UPSTREAM_BASELINE`
remains RED for physical conservation/positivity, VRAM and accepted performance
budget work, not because the current official source is absent.
