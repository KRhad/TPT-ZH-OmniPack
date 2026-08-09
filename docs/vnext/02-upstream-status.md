# Upstream status

## Verified current references

Refreshed on 2026-08-10 against the official download page, GitHub release
metadata and official Git remote:

```text
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_STABLE_REFERENCE=refs/tags/v100.1.400
UPSTREAM_TAG_OBJECT=c8be5165b7289706e17473bcfb38de112e78f140
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_MASTER_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
LOCAL_HEAD=430b3bd2868c17ba3d15c9a4580289173bcc79a6
LOCAL_UPSTREAM_BASE=bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd
```

Primary references:

- official build page: <https://powdertoy.co.uk/Download/Build.html?ID=400>
- official repository: <https://github.com/The-Powder-Toy/The-Powder-Toy>
- official tag: <https://github.com/The-Powder-Toy/The-Powder-Toy/releases/tag/v100.1.400>

The official download page returned HTTP 200 and displayed `100.1`; GitHub's
latest-release metadata reports non-prerelease `v100.1.400` targeting `master`.
`git ls-remote official` returned the tag object and its peeled commit above, and
official `master` resolved to the same commit. The release was published
2026-08-08T03:15:47Z. Search-engine caches were not used as the sole stable
authority.

## Remote policy

The correct official remote already existed as `official`; it was reused without
overwriting any remote. `git fetch official --prune` passed. A tag-inclusive fetch
safely rejected replacement of the unrelated local historical `v99.5.394` tag; it
was not forced and does not affect the independently verified 100.1 reference.
Future phase starts must fetch or use `ls-remote` again and compare stable and
master before continuing.

## Local relation to upstream

- Before adaptation, `fb72d5e8f` had 181 local-only commits and lacked 13 official
  commits relative to stable/master.
- The common base was `bff38ce6959e1c1a7a4d17d0d5d44d127a0dfcbd`.
- Current stable/master is an ancestor of `integration/omnicore-vnext`.
- Current integration has 214 commits beyond stable and is behind it by zero commits.
- `git rev-list ba30cd2e6..official/master` is zero: no upstream commits arrived
  after the local 100.1 merge.

This establishes source ancestry, not the complete G0 test/benchmark gate.
