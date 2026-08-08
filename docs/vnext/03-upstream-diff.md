# Upstream diff snapshot

The canonical four-way analysis is [`upstream-diff.md`](upstream-diff.md). This
numbered report fixes the first-round conclusions to the current integration point.

| Comparison | Local-only | Other-only | Meaning |
|---|---:|---:|---|
| common base `bff38ce` vs pre-vNext `fb72d5e8f` | 181 | 0 | OmniPack development |
| pre-vNext `fb72d5e8f` vs stable `d768aeb89` | 181 | 13 | fork had local work and lacked 13 upstream commits |
| current `f1320b48d` vs stable/master | 186 | 0 | upstream now integrated; five bounded vNext commits follow it |

All 13 previously missing upstream commits are classified in the canonical report.
The upstream merge is present, but `G0_UPSTREAM_BASELINE=RED` until performance,
characterization, strict-FP, and differential baselines exist.
