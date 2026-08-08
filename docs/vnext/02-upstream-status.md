# Upstream status

## Verified current references

Checked on 2026-08-09 against the official website and the official Git remote:

```text
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_STABLE_REFERENCE=refs/tags/v100.1.400
UPSTREAM_TAG_OBJECT=c8be5165b7289706e17473bcfb38de112e78f140
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
LOCAL_HEAD=f1320b48dfd5a570edc353a6d52dcd8adc094345
LOCAL_UPSTREAM_BASE=bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd
```

Primary references:

- official build page: <https://powdertoy.co.uk/Download/Build.html?ID=400>
- official repository: <https://github.com/The-Powder-Toy/The-Powder-Toy>
- official tag: <https://github.com/The-Powder-Toy/The-Powder-Toy/releases/tag/v100.1.400>

The direct build page returned `Version 100.1, Build 400`. `git ls-remote official`
returned the tag object and its peeled commit above, and official `master` resolved to
the same commit. Search-engine caches and the top-level download page may still show
100.0; they were not used as the sole stable authority.

## Remote policy

The correct official remote already existed as `official`; it was reused without
overwriting any remote. Future phase starts must fetch or use `ls-remote` again and
compare stable and master before continuing.

## Local relation to upstream

- Before adaptation, `fb72d5e8f` had 181 local-only commits and lacked 13 official
  commits relative to stable/master.
- The common base was `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd`.
- Current stable/master is an ancestor of `integration/omnicore-vnext`.
- Current integration has 186 commits beyond stable and is behind it by zero commits.

This establishes source ancestry, not the complete G0 test/benchmark gate.
