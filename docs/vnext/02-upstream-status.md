# Upstream status

## Verified current references

Refreshed on 2026-08-16 against the official download page, GitHub release
metadata and official Git remote. The values below are the current verified
references used by the release audit; they must be refreshed again before a
future stable promotion:

```text
UPSTREAM_STABLE_VERSION=100.1 build 400
UPSTREAM_STABLE_REFERENCE=refs/tags/v100.1.400
UPSTREAM_TAG_OBJECT=c8be5165b7289706e17473bcfb38de112e78f140
UPSTREAM_STABLE_COMMIT=d768aeb89acad986bd252d7e904bf44bb374545f
UPSTREAM_STABLE_BRANCH_COMMIT=cde24af43ffe440e4ea4481d4c1b47bca8ea41c0
UPSTREAM_STABLE_BRANCH_COMMIT_TIME=2018-04-02T23:04:33-04:00
UPSTREAM_MASTER_COMMIT=2e47966b84b0d2f1750af0f82643791803537ea5
LOCAL_HEAD_AT_REFRESH=8151e89a04239feb07821e843e5483a245a13455
LOCAL_UPSTREAM_BASE_AT_REFRESH=d768aeb89acad986bd252d7e904bf44bb374545f
LOCAL_ONLY_COMMITS_AT_REFRESH=344
UPSTREAM_ONLY_COMMITS_AT_REFRESH=1
```

Primary references:

- official build page: <https://powdertoy.co.uk/Download/Build.html?ID=400>
- official repository: <https://github.com/The-Powder-Toy/The-Powder-Toy>
- official tag: <https://github.com/The-Powder-Toy/The-Powder-Toy/releases/tag/v100.1.400>

The official download page returned HTTP 200 and displayed `100.1`; GitHub's
latest-release metadata reports non-prerelease `v100.1.400` targeting `master`.
`git ls-remote official` returned tag object
`c8be5165b7289706e17473bcfb38de112e78f140` peeled to stable commit
`d768aeb89acad986bd252d7e904bf44bb374545f`, while official `master` resolved to
`2e47966b84b0d2f1750af0f82643791803537ea5` (`Add destructibility view`, commit
time `2026-08-14T10:42:03+07:00`). The release was published
`2026-08-08T03:15:47Z`. Search-engine caches were not used as the sole stable
authority.
The remote branch named `stable` resolves separately to the legacy 2018 commit
`cde24af43ffe440e4ea4481d4c1b47bca8ea41c0` (`update comment`); it is not used as
the current release baseline. The tagged release and `master` are therefore
recorded independently above.

## Official save-fixture availability

The verified trees for both `refs/tags/v100.1.400` and `official/master` contain
zero `.cps` or `.stm` files. No official save corpus is therefore bundled or
claimed by this repository. `OfficialTPTCorpusProvenance` and
`OfficialTPTSaveCompatibility` remain `NOT_TESTED` until legally redistributable
fixtures can be injected from an external, provenance-checked corpus. A local
synthetic or OmniPack-generated save is not evidence for either gate.

## Remote policy

The correct official remote already existed as `official`; it was reused without
overwriting any remote. `git fetch official --prune` passed. A tag-inclusive fetch
safely rejected replacement of the unrelated local historical `v99.5.394` tag; it
was not forced and does not affect the independently verified 100.1 reference.
Future phase starts must fetch or use `ls-remote` again and compare stable and
master before continuing.

## Local relation to upstream

- At refresh snapshot `8151e89a0`, stable commit `d768aeb89...` was the
  merge-base and an ancestor of `integration/omnicore-vnext`.
- That snapshot had 344 local-only commits relative to `official/master` and was
  behind it by 1 commit. The upstream-only commit is the post-release `Add
  destructibility view` change on `official/master`; it has not been silently
  merged into the release branch.

This establishes source ancestry, not the complete G0 test/benchmark gate.
